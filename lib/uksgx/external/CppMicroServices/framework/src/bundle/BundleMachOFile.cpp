/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#include "BundleObjFile.h"

#include "cppmicroservices_mach-o.h"

#include <cerrno>
#include <cstring>
#include <fstream>

#include <sys/stat.h>

namespace cppmicroservices {

struct InvalidMachOException : public InvalidObjFileException
{
  InvalidMachOException(const std::string& what, int errorNumber = 0)
    : InvalidObjFileException(what, errorNumber)
  {}
};

template<uint32_t>
struct MachO;

template<>
struct MachO<MH_MAGIC>
{
  typedef mach_header Mhdr;
  typedef nlist symtab_entry;
};

template<>
struct MachO<MH_MAGIC_64>
{
  typedef mach_header_64 Mhdr;
  typedef nlist_64 symtab_entry;
};

template<typename I>
I readBE(I i)
{
#ifdef US_LITTLE_ENDIAN
  I r = 0;
  for (unsigned int n = 0; n < sizeof(I); ++n) {
    r |= static_cast<I>(*(reinterpret_cast<unsigned char*>(&i) + n))
         << ((sizeof(I) - 1 - n) * 8);
  }
  return r;
#else
  return i;
#endif
}

template<class MachOType>
class BundleMachOFile
  : public BundleObjFile
  , private MachOType
{
public:
  typedef typename MachOType::Mhdr Mhdr;
  typedef typename MachOType::symtab_entry symtab_entry;

  BundleMachOFile(std::ifstream& fs, std::size_t fileOffset)
  {
    fs.seekg(fileOffset);
    Mhdr mhdr;
    fs.read(reinterpret_cast<char*>(&mhdr), sizeof mhdr);
    if (mhdr.filetype != MH_DYLIB && mhdr.filetype != MH_BUNDLE) {
      throw InvalidMachOException(
        "Not a Mach-O dynamic shared library or bundle file.");
    }

    // iterate over all load commands
    uint32_t ncmds = mhdr.ncmds;
    uint32_t lcmd_offset = static_cast<uint32_t>(fs.tellg());
    uint32_t symtab_offset = 0;
    uint32_t strtab_offset = 0;
    uint32_t strtab_size = 0;
    for (uint32_t i = 0; i < ncmds; ++i) {
      load_command lcmd;
      fs.read(reinterpret_cast<char*>(&lcmd), sizeof lcmd);
      if (lcmd.cmd == LC_ID_DYLIB) {
        dylib dylib_id;
        fs.read(reinterpret_cast<char*>(&dylib_id), sizeof dylib_id);
        PrintVersionString(dylib_id.current_version);
        PrintVersionString(dylib_id.compatibility_version);
        fs.seekg(lcmd_offset + dylib_id.name.offset);
        std::getline(fs, m_InstallName, '\0');
      } else if (lcmd.cmd == LC_LOAD_DYLIB) {
        dylib dylib_id;
        fs.read(reinterpret_cast<char*>(&dylib_id), sizeof dylib_id);
        fs.seekg(lcmd_offset + dylib_id.name.offset);
        std::string id;
        std::getline(fs, id, '\0');
        m_Needed.push_back(id);
      } else if (lcmd.cmd == LC_SYMTAB) {
        fs.seekg(-2 * static_cast<int>(sizeof(uint32_t)), std::ios_base::cur);
        symtab_command symtab;
        fs.read(reinterpret_cast<char*>(&symtab), sizeof symtab);
        symtab_offset = symtab.symoff;
        strtab_offset = symtab.stroff;
        strtab_size = symtab.strsize;
      } else if (lcmd.cmd == LC_DYSYMTAB) {
        fs.seekg(-2 * static_cast<int>(sizeof(uint32_t)), std::ios_base::cur);
        dysymtab_command dysymtab;
        fs.read(reinterpret_cast<char*>(&dysymtab), sizeof dysymtab);
        if (dysymtab.nextdefsym != 0) {
          // read the defined external symbol table entries
          symtab_entry* extSyms = new symtab_entry[dysymtab.nextdefsym];
          fs.seekg(fileOffset + symtab_offset +
                   dysymtab.iextdefsym * sizeof(symtab_entry));
          fs.read(reinterpret_cast<char*>(extSyms),
                  dysymtab.nextdefsym * sizeof(symtab_entry));

          // read the string table
          char* strtab = new char[strtab_size];
          fs.seekg(fileOffset + strtab_offset);
          fs.read(strtab, strtab_size);

          // iterate over all defined external symbol table entries
          symtab_entry* extSym = extSyms;
          for (std::size_t i = 0; i < dysymtab.nextdefsym; ++i, ++extSym) {
            if (static_cast<uint32_t>(extSym->n_un.n_strx) >= strtab_size)
              continue;
            // strip of the leading "_" in the symbol name
            if (this->ExtractBundleName(&strtab[extSym->n_un.n_strx + 1],
                                        m_BundleName)) {
              break;
            }
          }
          delete[] strtab;
          delete[] extSyms;
        }
      }

      lcmd_offset += lcmd.cmdsize;
      fs.seekg(lcmd_offset);
    }
  }

  virtual std::vector<std::string> GetDependencies() const { return m_Needed; }

  virtual std::string GetLibName() const { return m_InstallName; }

  virtual std::string GetBundleName() const { return m_BundleName; }

private:
  std::vector<std::string> m_Needed;
  std::string m_InstallName;
  std::string m_BundleName;

  void PrintVersionString(uint32_t /*version*/)
  {
    /*
    uint32_t pv = version & 0xff;
    uint32_t minor = (version & 0xff00) >> 8;
    uint32_t major = (version & 0xff0000) >> 16;
    std::cout << major << "." << minor << "." << pv << std::endl;
    */
  }
};

static std::vector<std::vector<uint32_t>> GetMachOIdents(std::ifstream& is)
{
  // magic (32 or 64 bit) | cputype | offset
  std::vector<std::vector<uint32_t>> idents;

  uint32_t magic;
  is.seekg(0);
  is.read(reinterpret_cast<char*>(&magic), sizeof magic);

  if (readBE(magic) == FAT_MAGIC) {
    is.seekg(0);
    fat_header fatHdr;
    is.read(reinterpret_cast<char*>(&fatHdr), sizeof fatHdr);
    fat_arch* fatArchs = new fat_arch[readBE(fatHdr.nfat_arch)];
    is.read(reinterpret_cast<char*>(fatArchs),
            sizeof *fatArchs * readBE(fatHdr.nfat_arch));
    fat_arch* currArch = fatArchs;
    for (uint32_t i = 0; i < readBE(fatHdr.nfat_arch); ++i, ++currArch) {
      is.seekg(readBE(currArch->offset));
      mach_header machHdr;
      is.read(reinterpret_cast<char*>(&machHdr), sizeof machHdr);
      std::vector<uint32_t> ident(3, 0);
      ident[0] = machHdr.magic;
      ident[1] = readBE(currArch->cputype);
      ident[2] = readBE(currArch->offset);
      idents.push_back(ident);
    }
  } else {
    std::vector<uint32_t> ident(3, 0);
    ident[0] = magic;
    is.read(reinterpret_cast<char*>(&ident[1]), sizeof(uint32_t));
    idents.push_back(ident);
  }
  return idents;
}

// We can't get the identification bits for the currently
// running bundle (the host system) directly from the file
// because the file could itself be a fat binary with multiple
// possible target systems.
static std::vector<uint32_t> GetMachOIdent()
{
  // magic (32 or 64 bit) | cputype | offset
  std::vector<uint32_t> ident(3, 0);

#ifdef __LP64__
  ident[0] = MH_MAGIC_64;
#else
  ident[0] = MH_MAGIC;
#endif

#if defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__)
  ident[1] = CPU_TYPE_POWERPC64;
#elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
  ident[1] = CPU_TYPE_POWERPC;
#elif defined(__sparc)
  ident[1] = CPU_TYPE_SPARC;
#elif defined(__x86_64__) || defined(_M_X64)
  ident[1] = CPU_TYPE_X86_64;
#elif defined(__i386) || defined(_M_IX86)
  ident[1] = CPU_TYPE_X86;
#elif defined(__arm__) || defined(_M_ARM)
  ident[1] = CPU_TYPE_ARM;
#endif

  return ident;
}

BundleObjFile* CreateBundleMachOFile(const char* /*selfName*/,
                                     const std::string& fileName)
{
  struct stat machStat;
  if (stat(fileName.c_str(), &machStat) != 0) {
    throw InvalidMachOException("Stat for " + fileName + " failed", errno);
  }

  std::size_t fileSize = machStat.st_size;

  // magic number
  if (fileSize < sizeof(uint32_t)) {
    throw InvalidMachOException("Missing magic number");
  }

  std::ifstream machFile(fileName.c_str(), std::ios_base::binary);
  machFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  std::vector<uint32_t> selfIdent = GetMachOIdent();
  std::vector<std::vector<uint32_t>> fileIdents = GetMachOIdents(machFile);

  std::vector<uint32_t> matchingIdent(3, 0);

  // check if the identifications match for the running application
  // and the Mach-O file (or one of the embedded files in the fat binary)
  for (std::size_t i = 0; i < fileIdents.size(); ++i) {
    if (memcmp(&fileIdents[i][0], &selfIdent[0], 2 * sizeof(uint32_t)) == 0) {
      matchingIdent = fileIdents[i];
    }
  }

  if (matchingIdent[0] == 0) {
    throw InvalidMachOException("Not a compatible Mach-O file or fat binary");
  }

  if (matchingIdent[0] == MH_MAGIC) {
    //std::cout << "Mach-O (32-bit) binary" << std::endl;
    return new BundleMachOFile<MachO<MH_MAGIC>>(machFile, matchingIdent[2]);
  } else if (matchingIdent[0] == MH_MAGIC_64) {
    //std::cout << "Mach-O (64-bit) binary" << std::endl;
    return new BundleMachOFile<MachO<MH_MAGIC_64>>(machFile, matchingIdent[2]);
  } else {
    throw InvalidMachOException(
      "Internal error: Mach-O magic is neither MH_MAGIC nor MH_MAGIC_64");
  }
}
}
