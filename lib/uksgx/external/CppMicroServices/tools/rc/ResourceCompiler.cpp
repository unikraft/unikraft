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

#include "miniz.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "optionparser.h"
#include "json/json.h"

// ---------------------------------------------------------------------------------
// --------------------------    PLATFORM SPECIFIC CODE    -------------------------
// ---------------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)

#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  include <windows.h>
#  define PATH_SEPARATOR "\\"
static std::string get_error_str()
{
  // Retrieve the system error message for the last-error code
  LPVOID lpMsgBuf;
  DWORD dw = GetLastError();
  std::string errMsg;
  DWORD rc =
    FormatMessageA((FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS),
                   NULL,
                   dw,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   reinterpret_cast<LPTSTR>(&lpMsgBuf),
                   0,
                   NULL);
  // If FormatMessage fails using FORMAT_MESSAGE_ALLOCATE_BUFFER
  // it means that the size of the error message exceeds an internal
  // buffer limit (128 kb according to MSDN) and lpMsgBuf will be
  // uninitialized.
  // Inform the caller that the error message couldn't be retrieved.
  if (rc == 0) {
    errMsg = "Failed to retrieve error message.";
  } else {
    errMsg = reinterpret_cast<LPCTSTR>(lpMsgBuf);
    LocalFree(lpMsgBuf);
  }
  return errMsg;
}

std::string us_tempfile()
{
  char szTempFileName[MAX_PATH];
  if (GetTempFileNameA(".", "ZIP", 1, szTempFileName) == 0) {
    std::cerr << "Error: " << get_error_str() << std::endl;
    exit(EXIT_FAILURE);
  }
  return std::string(szTempFileName);
}

#else

#  include <unistd.h>
#  define PATH_SEPARATOR "/"
static std::string get_error_str()
{
  return strerror(errno);
}

std::string us_tempfile()
{
  char temppath[] = "./ZIP_XXXXXX";
  if (mkstemp(temppath) == -1) {
    std::cerr << "Error: " << get_error_str() << std::endl;
    exit(EXIT_FAILURE);
  }
  return std::string(temppath);
}

#endif

// ---------------------------------------------------------------------------------
// ------------------------   END PLATFORM SPECIFIC CODE    ------------------------
// ---------------------------------------------------------------------------------

namespace {

class InvalidManifest : public std::runtime_error
{
public:
  InvalidManifest(const std::string& msg)
    : std::runtime_error(msg)
  {}
  InvalidManifest(const char* msg)
    : std::runtime_error(msg)
  {}
};

/*
 * @brief parses json content and returns the parsed json or throws.
 * @tparam jsonContent json content to parse. The type must be convertible to std::istream.
 * @param root The parsed Json root object.
 * @throw InvalidManifest if the json is invalid. Parse error information is in the exception.
 * If an exception is thrown, the root param is invalid.
 */
template<class T>
void parseAndValidateJson(T& jsonContent, Json::Value& root)
{
  Json::CharReaderBuilder rbuilder;
  rbuilder["rejectDupKeys"] = true;
  rbuilder["allowComments"] = false;
  std::string errs;

  if (!Json::parseFromStream(rbuilder, jsonContent, &root, &errs)) {
    throw InvalidManifest(errs);
  }
}

/*
 * @brief parses json content from a file and returns the parsed json or throws.
 * @param jsonFile path to a json file.
 * @param root The parsed Json root object.
 * @throw InvalidManifest if the json is invalid. Parse error information is in the exception.
 * If an exception is thrown, the root param is invalid.
 */
void parseAndValidateJsonFromFile(const std::string& jsonFile,
                                  Json::Value& root)
{
  try {
    std::ifstream json(jsonFile);
    if (!json.is_open()) {
      throw std::runtime_error("Could not open file " + jsonFile);
    }
    parseAndValidateJson(json, root);
  } catch (const InvalidManifest& e) {
    std::string exceptionMsg(jsonFile + ": " + e.what());
    throw InvalidManifest(exceptionMsg);
  }
}

/*
 * @brief extracts a manifest file from the zip archive and checks for correct JSON syntax.
 * @param zipArchive miniz data structure representing the opened zip archive.
 * @param archiveFileName file path of the zip archive.
 * @param archiveEntry archive entry path of the manifest file.
 * @throw InvalidManifest if the json is invalid. Parse error information is in the exception.
 * @throw runtime_error if the manifest file could not be read from the archive.
 */
void validateManifestInArchive(mz_zip_archive* zipArchive,
                               const std::string& archiveFile,
                               const std::string& archiveEntry)
{
  void* data = mz_zip_reader_extract_file_to_heap(
    zipArchive, archiveEntry.c_str(), nullptr, 0);
  std::unique_ptr<void, void (*)(void*)> manifestFileContents(data, ::free);

  if (!manifestFileContents) {
    throw std::runtime_error("Failed to extract " + archiveEntry + " from " +
                             archiveFile);
  }

  try {
    Json::Value root;
    std::istringstream json(
      reinterpret_cast<const char*>(manifestFileContents.get()));
    parseAndValidateJson(json, root);
  } catch (const InvalidManifest& e) {
    std::string exceptionMsg(archiveFile);
    exceptionMsg += " (" + archiveEntry + ") : " + e.what();
    throw InvalidManifest(exceptionMsg);
  }
}

/* 
 * @brief Validate manifest files in an archive.
 * @param archiveFile archive file path
 * @throw InvalidManifest on the first invalid manifest found.
 * @throw runtime_error on the first manifest file which could not be read from the archive.
 */
void validateManifestsInArchive(const std::string& archiveFile)
{
  mz_zip_archive currZipArchive;
  mz_uint currZipIndex = 0;
  memset(&currZipArchive, 0, sizeof(mz_zip_archive));
  std::clog << "Validating manifests in " << archiveFile << " ... "
            << std::endl;
  if (!mz_zip_reader_init_file(&currZipArchive, archiveFile.c_str(), 0)) {
    throw std::runtime_error("Could not initialize zip archive " + archiveFile);
  }
  char archiveName[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
  mz_uint numZipIndices = mz_zip_reader_get_num_files(&currZipArchive);
  for (currZipIndex = 0; currZipIndex < numZipIndices; ++currZipIndex) {
    mz_uint numBytes = mz_zip_reader_get_filename(
      &currZipArchive, currZipIndex, archiveName, sizeof archiveName);
    std::clog << "\tValidating: " << archiveName << " (from " << archiveFile
              << ") " << std::endl;
    try {
      if (numBytes > 1 && archiveName[numBytes - 2] !=
                            '/') // The last character is '\0' in the array
      {
        std::string archiveEntry(archiveName);
        if (archiveEntry.find_first_of("/", 0) ==
              archiveEntry.find_last_of("/") &&
            std::string::npos != archiveEntry.find("/manifest.json")) {
          validateManifestInArchive(&currZipArchive, archiveFile, archiveEntry);
        }
      }
    } catch (const std::exception&) {
      mz_zip_reader_end(&currZipArchive);
      throw;
    }
  }
  std::clog << "Finished validating manifest files from " << archiveName
            << std::endl;
  mz_zip_reader_end(&currZipArchive);
}

/*
 * @brief concatenates all manifests checking for invalid syntax and
 * duplicate JSON key names.
 * @param manifests a map containing manifest file paths and their content.
 * @pre-condition each manifest in manifests has already been validated by jsoncpp.
 * @throw InvalidManifest if the manifest has inavlid syntax or duplicate key names
 * @return valid JSON content
 */
Json::Value AggregateManifestsAndValidate(
  std::unordered_map<std::string, Json::Value>& manifests)
{
  Json::Value root;

  for (auto& manifest : manifests) {
    Json::Value jsonRoot(manifest.second);
    Json::Value::iterator iter = jsonRoot.begin();
    for (; iter != jsonRoot.end(); ++iter) {
      // concatenating all the root objects together means that
      // duplicate key names only have to be checked for children of
      // each root object. Any other duplicate keys would have been
      // detected when validating each individual JSON file.
      if (Json::Value{} == root[iter.name()]) {
        root[iter.name()] = (*iter);
      } else {
        throw InvalidManifest(std::string("Duplicate key: '" + iter.name() +
                                          "' found in " + manifest.first));
      }
    }
  }

  std::clog << "concatenated (pre-validated) json:\n"
            << root.toStyledString() << std::endl;

  // Any duplicate keys would have been flagged earlier while concatenating all the manifest files.
  // This is a final JSON validation which should only find JSON syntax errors caused by an error in
  // concatenation.
  Json::Value manifestJson;
  std::istringstream json(root.toStyledString());
  parseAndValidateJson(json, manifestJson);

  std::clog << "final (validated) manifest.json:\n"
            << manifestJson.toStyledString() << std::endl;
  return manifestJson;
}
}

/*
 *@brief class to represent the zip archive for bundles
 */
class ZipArchive
{
public:
  ZipArchive(const std::string& archiveFileName,
             int compressionLevel,
             const std::string& bundleName);
  virtual ~ZipArchive();
  /*
  * @brief Add manifest.json to this zip archive
  * @param manifest contents of the manifest to add to the zip archive
  * @throw std::runtime exception if failed to add manifest.json
  * @throw InvalidManifest if manifest.json is invalid
  */
  void AddManifestFile(const Json::Value& manifest);

  /*
   * @brief Add a file to this zip archive
   * @throw std::runtime exception if failed to add the resource file
   * @throw InvalidManifest if manifest.json is invalid
   * @param resFileName is the path to the resource to be added
   * @param isManifest indicates if the file is the bundle's manifest
   */
  void AddResourceFile(const std::string& resFileName, bool isManifest = false);

  /*
   * @brief Add all files from another zip archive to this zip archive
   * @throw std::runtime exception if failed to add any of the resources
   * @param archiveFileName is the path to the source archive
   */
  void AddResourcesFromArchive(const std::string& archiveFileName);

  // Remove copy constructor and assignment
  ZipArchive(const ZipArchive&) = delete;
  void operator=(const ZipArchive&) = delete;
  // Remove move constructor and assignment
  ZipArchive(ZipArchive&&) = delete;
  ZipArchive& operator=(ZipArchive&&) = delete;

private:
  /*
   * @brief Add a directory entry to the zip archive
   * @throw std::runtime exception if failed to add the entry
   */
  void AddDirectory(const std::string& dirName);

  /*
   * @brief Checks whether the archive file entry already
   *        exists in the zip archive. If it does, throw an exception.
   * @throw std::runtime_error if the archive entry already exists.
   */
  void CheckAndAddToArchivedNames(const std::string& archiveEntry);

  void PrintErrorAndExit(const std::string& errorMsg)
  {
    std::cerr << errorMsg << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string fileName;
  int compressionLevel;
  std::string bundleName;
  std::unique_ptr<mz_zip_archive> writeArchive;
  std::set<std::string> archivedNames; // list of all the file entries
  std::set<std::string> archivedDirs;  // list of all directory entries
};

ZipArchive::ZipArchive(const std::string& archiveFileName,
                       int compressionLevel,
                       const std::string& bName)
  : fileName(archiveFileName)
  , compressionLevel(compressionLevel)
  , bundleName(bName)
  , writeArchive(new mz_zip_archive())
{
  std::clog << "Initializing zip archive " << fileName << " ..." << std::endl;
  // clear the contents of a outFile if it exists
  std::ofstream ofile(fileName, std::ofstream::trunc);
  ofile.close();
  if (!mz_zip_writer_init_file(writeArchive.get(), fileName.c_str(), 0)) {
    throw std::runtime_error("Internal error, could not init new zip archive");
  }
}

void ZipArchive::CheckAndAddToArchivedNames(const std::string& archiveEntry)
{
  std::clog << "Adding file " << archiveEntry << " ..." << std::endl;
  // add the current file to the new archive
  if (!archivedNames.insert(archiveEntry).second) {
    throw std::runtime_error("A file already exists with the name " +
                             archiveEntry);
  }
}

void ZipArchive::AddManifestFile(const Json::Value& manifest)
{
  std::string styledManifestJson(manifest.toStyledString());
  std::string archiveEntry(bundleName + "/manifest.json");

  CheckAndAddToArchivedNames(archiveEntry);

  if (MZ_FALSE == mz_zip_writer_add_mem(writeArchive.get(),
                                        archiveEntry.c_str(),
                                        styledManifestJson.c_str(),
                                        styledManifestJson.size(),
                                        compressionLevel)) {
    throw std::runtime_error("Error writing manifest.json to archive " +
                             fileName);
  }
  AddDirectory(bundleName + "/");
}

void ZipArchive::AddResourceFile(const std::string& resFileName,
                                 bool isManifest)
{
  std::string archiveName = resFileName;

  // This check exists solely to maintain a deprecated way of adding manifest.json
  // through the --res-add option.
  if (isManifest || resFileName == std::string("manifest.json")) {
    Json::Value root;
    parseAndValidateJsonFromFile(resFileName, root);
  }

  // if it is a manifest file, we ignore the parent directory path because the
  // manifest file is always placed at the root of the bundle name directory
  if (isManifest &&
      resFileName.find_last_of(PATH_SEPARATOR) != std::string::npos) {
    archiveName =
      resFileName.substr(resFileName.find_last_of(PATH_SEPARATOR) + 1);
  }

  std::string archiveEntry = bundleName + "/" + archiveName;
  CheckAndAddToArchivedNames(archiveEntry);

  if (!mz_zip_writer_add_file(writeArchive.get(),
                              archiveEntry.c_str(),
                              resFileName.c_str(),
                              NULL,
                              0,
                              compressionLevel)) {
    throw std::runtime_error("Error writing file to archive");
  }
  // add a directory entries for the file path
  size_t lastPathSeparatorPos = archiveEntry.find("/", 0);
  while (lastPathSeparatorPos != std::string::npos) {
    AddDirectory(archiveEntry.substr(0, lastPathSeparatorPos + 1));
    lastPathSeparatorPos = archiveEntry.find("/", lastPathSeparatorPos + 1);
  }
}

void ZipArchive::AddDirectory(const std::string& dirName)
{
  assert(dirName[dirName.length() - 1] == '/');
  if (archivedDirs.insert(dirName).second) {
    std::clog << "\t new dir entry " << dirName << std::endl;
    // The directory entry does not yet exist, so add it
    if (!mz_zip_writer_add_mem(
          writeArchive.get(), dirName.c_str(), NULL, 0, MZ_NO_COMPRESSION)) {
      throw std::runtime_error("zip add_mem error");
    }
  }
}

ZipArchive::~ZipArchive()
{
  assert(writeArchive->m_zip_mode != MZ_ZIP_MODE_INVALID);
  std::clog << "Finalizing the zip archive ..." << std::endl;
  std::clog << "Archive has the following files" << std::endl;
  for (auto fileNameEntry : archivedNames) {
    std::clog << "\t " << fileNameEntry << std::endl;
  }
  std::clog << "and directory entries" << std::endl;
  for (auto dirEntry : archivedDirs) {
    std::clog << "\t " << dirEntry << std::endl;
  }
  if (mz_zip_writer_finalize_archive(writeArchive.get()) == MZ_FALSE) {
    PrintErrorAndExit("Failed to finalize Zip archive");
  }
  // check state after finalizing the archive.
  assert(writeArchive->m_zip_mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED);
  if (mz_zip_writer_end(writeArchive.get()) == MZ_FALSE) {
    PrintErrorAndExit("Failed to close Zip archive");
  }
  // check state after closing the archive file.
  assert(writeArchive->m_zip_mode == MZ_ZIP_MODE_INVALID);
}

void ZipArchive::AddResourcesFromArchive(const std::string& archiveFileName)
{
  mz_zip_archive currZipArchive;
  mz_uint currZipIndex = 0;
  memset(&currZipArchive, 0, sizeof(mz_zip_archive));
  std::clog << "Merging zip file " << archiveFileName << " ... " << std::endl;
  if (!mz_zip_reader_init_file(&currZipArchive, archiveFileName.c_str(), 0)) {
    throw std::runtime_error("Could not initialize zip archive " +
                             archiveFileName);
  }
  char archiveName[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
  mz_uint numZipIndices = mz_zip_reader_get_num_files(&currZipArchive);
  for (currZipIndex = 0; currZipIndex < numZipIndices; ++currZipIndex) {
    mz_uint numBytes = mz_zip_reader_get_filename(
      &currZipArchive, currZipIndex, archiveName, sizeof archiveName);
    std::clog << "\tmerging: " << archiveName << " (from " << archiveFileName
              << ") " << std::endl;
    try {
      if (numBytes > 1) {
        if (archiveName[numBytes - 2] !=
            '/') // The last character is '\0' in the array
        {
          if (!archivedNames.insert(archiveName).second) {
            throw std::runtime_error("Found duplicate file with name " +
                                     std::string(archiveName));
          }

          std::string archiveEntry(archiveName);
          if (archiveEntry.find_first_of("/", 0) ==
                archiveEntry.find_last_of("/") &&
              std::string::npos != archiveEntry.find("/manifest.json")) {
            validateManifestInArchive(
              &currZipArchive, archiveFileName, archiveEntry);
          }

          if (!mz_zip_writer_add_from_zip_reader(
                writeArchive.get(), &currZipArchive, currZipIndex)) {
            throw std::runtime_error("Failed to append file " +
                                     std::string(archiveName) +
                                     " from archive " + archiveFileName);
          }
        } else {
          AddDirectory(std::string(archiveName));
        }
      }
    } catch (const std::exception&) {
      mz_zip_reader_end(&currZipArchive);
      throw;
    }
  }
  std::clog << "Finished merging files from " << archiveFileName << std::endl;
  mz_zip_reader_end(&currZipArchive);
}

struct Custom_Arg : public option::Arg
{
  static void printError(const std::string& msg1,
                         const option::Option& opt,
                         const std::string& msg2)
  {
    std::cerr << "ERROR: " << msg1 << opt.name << msg2 << std::endl;
  }

  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0) {
      return option::ARG_OK;
    }
    if (msg) {
      printError("Option '", option, "' requires a non-empty argument\n");
    }
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = nullptr;
    if (option.arg != nullptr) {
      errno = 0;
      // the return value of strtol is misleading since 0 indicates failure and
      // we support 0 as a valid command line option argument. Checking errno
      // is the correct way to determine if the argument is valid.
      long ret = strtol(option.arg, &endptr, 10);
      (void)ret;
      if (errno != ERANGE) {
        errno = 0;
        assert(endptr != nullptr);
        return option::ARG_OK;
      }
    }

    if (msg) {
      printError("Option '", option, "' requires a numeric argument\n");
    }
    return option::ARG_ILLEGAL;
  }
};

// $TODO We need to get the executable name at runtime
#define US_PROG_NAME "usResourceCompiler3"

enum OptionIndex
{
  UNKNOWN,
  HELP,
  VERBOSE,
  BUNDLENAME,
  COMPRESSIONLEVEL,
  OUTFILE,
  RESADD,
  ZIPADD,
  MANIFESTADD,
  BUNDLEFILE
};

const option::Descriptor usage[] = {
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\nUSAGE: " US_PROG_NAME " [options]\n\n"
    "Options:" },
  { HELP,
    0,
    "h",
    "help",
    Custom_Arg::None,
    " --help, -h  \tPrint usage and exit." },
  { VERBOSE,
    0,
    "V",
    "verbose",
    Custom_Arg::None,
    " --verbose, -V  \tRun in verbose mode." },
  { BUNDLENAME,
    0,
    "n",
    "bundle-name",
    Custom_Arg::NonEmpty,
    " --bundle-name, -n \tThe bundle name as specified in the US_BUNDLE_NAME "
    "compile definition." },
  { COMPRESSIONLEVEL,
    0,
    "c",
    "compression-level",
    Custom_Arg::Numeric,
    " --compression-level, -c  \tCompression level used for zip. Value range "
    "is 0 to 9. Default value is 6." },
  { OUTFILE,
    0,
    "o",
    "out-file",
    Custom_Arg::NonEmpty,
    " --out-file, -o \tPath to output zip file. If the file exists it will be "
    "overwritten. If this option is not provided, a temporary zip fie will be "
    "created." },
  { RESADD,
    0,
    "r",
    "res-add",
    Custom_Arg::NonEmpty,
    " --res-add, -r \tPath to a resource file, relative to the current working "
    "directory." },
  { ZIPADD,
    0,
    "z",
    "zip-add",
    Custom_Arg::NonEmpty,
    " --zip-add, -z \tPath to a file containing a zip archive to be merged "
    "into the output zip file. " },
  { MANIFESTADD,
    0,
    "m",
    "manifest-add",
    Custom_Arg::NonEmpty,
    " --manifest-add, -m \tPath to a bundle manifest file. Multiple bundle "
    "manifests will be concatenated together into one." },
  { BUNDLEFILE,
    0,
    "b",
    "bundle-file",
    Custom_Arg::NonEmpty,
    " --bundle-file, -b \tPath to the bundle binary. The resources zip file "
    "will be appended to this binary. " },
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\nNote:\n1. Only options --res-add and --zip-add can be specified "
    "multiple times." },
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\n2. If option --manifest-add or --res-add is specified, option "
    "--bundle-name must be provided." },
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\n3. At-least one of --bundle-file or --out-file options must be "
    "provided." },
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\nExamples:\n\nCreate a zip file with resources:\n"
    "  " US_PROG_NAME
    " --compression-level 9 --verbose --bundle-name mybundle --out-file "
    "Example.zip --manifest-add manifest.json --zip-add filetomerge.zip\n"
    "Behavior: Construct a zip blob with contents 'mybundle/manifest.json', "
    "merge the contents of zip file 'filetomerge.zip' into it and write the "
    "resulting blob into 'Example.zip'\n" },
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\nAppend a bundle with resources\n"
    "  " US_PROG_NAME
    " -V -n mybundle -b mybundle.dylib -m manifest.json -z archivetomerge.zip\n"
    "Behavior: Construct a zip blob with contents 'mybundle/manifest.json', "
    "merge the contents of zip file 'archivetomerge.zip' into it and append "
    "the resulting zip blob to 'mybundle.dylib'\n" },
  { UNKNOWN,
    0,
    "",
    "",
    Custom_Arg::None,
    "\nAppend a bundle binary with existing zip file\n"
    "  " US_PROG_NAME ".exe -b mybundle.dll -z archivetoembed.zip\n"
    "Behavior: Append the contents of 'archivetoembed.zip' to "
    "'mybundle.dll'\n" },
  { 0, 0, 0, 0, 0, 0 }
};

// Check invalid invocations and errors
static int checkSanity(option::Parser& parse, option::Option* options)
{
  int return_code = EXIT_SUCCESS;

  // Check if parsing command line resulted in a failure
  if (parse.error()) {
    std::cerr << "Parsing command line arguments failed. " << std::endl;
    return_code = EXIT_FAILURE;
  }

  // Check for unrecognized options
  if (parse.nonOptionsCount()) {
    std::clog << "unrecognized options ..." << std::endl;
    for (int i = 0; i < parse.nonOptionsCount(); ++i) {
      std::cout << "\t" << parse.nonOption(i) << std::endl;
    }
    return_code = EXIT_FAILURE;
  }

  // Multiple specifications of the following arguments are illegal
  auto check_multiple_args =
    [&options, &return_code](std::initializer_list<OptionIndex> optionIndices) {
      for (auto optionidx : optionIndices) {
        option::Option* opt = options[optionidx];
        if (opt && opt->count() > 1) {
          std::cerr << opt->name;
          std::cerr << " appears multiple times in the arguments. Check usage."
                    << std::endl;
          return_code = EXIT_FAILURE;
        }
      }
    };
  check_multiple_args({ BUNDLEFILE, OUTFILE, BUNDLENAME });

  // At-least one of --bundle-file or --out-file is required.
  if (!options[BUNDLEFILE] && !options[OUTFILE]) {
    std::cerr << "At least one of the options (--bundle-file | --out-file) is "
                 "required. Check usage."
              << std::endl;
    return_code = EXIT_FAILURE;
  }

  // If either --manifest-add or --res-add is given, --bundle-name must also be given.
  if ((options[MANIFESTADD] || options[RESADD]) && !options[BUNDLENAME]) {
    std::cerr << "If either --manifest-add or --res-add is provided, "
                 "--bundle-name must be provided."
              << std::endl;
    return_code = EXIT_FAILURE;
  }

  // Generate a warning that --bundle-name is not necessary in following invocation.
  if (options[BUNDLENAME] && !options[MANIFESTADD] && !options[RESADD] &&
      return_code != EXIT_FAILURE) {
    std::clog << "Warning: --bundle-name option is unnecessary here."
              << std::endl;
  }

  return return_code;
}

// ---------------------------------------------------------------------------------
// -----------------------------    MAIN ENTRY POINT    ----------------------------
// ---------------------------------------------------------------------------------

int main(int argc, char** argv)
{
  const int BUNDLE_MANIFEST_VALIDATION_ERROR_CODE(2);

  int compressionLevel = MZ_DEFAULT_LEVEL; //default compression level;
  int return_code = EXIT_SUCCESS;
  std::string bundleName;

  argc -= (argc > 0);
  argv += (argc > 0); // skip program name argv[0]
  option::Stats stats(usage, argc, argv);
  std::unique_ptr<option::Option[]> options(
    new option::Option[stats.options_max]);
  std::unique_ptr<option::Option[]> buffer(
    new option::Option[stats.buffer_max]);
  option::Parser parse(true, usage, argc, argv, options.get(), buffer.get());

  if (argc == 0 || options[HELP]) {
    option::printUsage(std::clog, usage);
    return return_code;
  }

  return_code = checkSanity(parse, options.get());
  if (return_code == EXIT_FAILURE) {
    return return_code;
  }

  if (options[BUNDLENAME]) {
    bundleName = options[BUNDLENAME].arg;
  }

  if (!options[VERBOSE]) {
    // if not in verbose mode, supress the clog stream
    std::clog.setstate(std::ios_base::failbit);
  }

  if (options[COMPRESSIONLEVEL]) {
    char* endptr = nullptr;
    compressionLevel = strtol(options[COMPRESSIONLEVEL].arg, &endptr, 10);
  }
  std::clog << "using compression level " << compressionLevel << std::endl;

  std::string zipFile;
  bool deleteTempFile = false;

  option::Option* bundleFileOpt = options[BUNDLEFILE];
  option::Option* outFileOpt = options[OUTFILE];

  try {
    // Append mode only works with one zip-add argument.
    if (!options[RESADD] && !options[MANIFESTADD] &&
        options[ZIPADD].count() == 1 && options[BUNDLEFILE]) {
      // jump to append part.
      zipFile = options[ZIPADD].arg;
    } else {
      if (outFileOpt) {
        zipFile = outFileOpt->arg;
      } else {
        zipFile = us_tempfile();
        deleteTempFile = true;
      }

      std::unique_ptr<ZipArchive> zipArchive(
        new ZipArchive(zipFile, compressionLevel, bundleName));

      // map of manifest file to its JSON data
      std::unordered_map<std::string, Json::Value> manifests;

      // Add the manifest file to zip archive
      if (options[MANIFESTADD]) {
        for (option::Option* opt = options[MANIFESTADD]; opt;
             opt = opt->next()) {
          Json::Value manifest;
          parseAndValidateJsonFromFile(std::string(opt->arg), manifest);
          bool result;
          std::tie(std::ignore, result) =
            manifests.insert(std::make_pair(opt->arg, manifest));
          if (!result) {
            std::clog << "Skipping duplicate manifest file " << opt->arg
                      << std::endl;
          }
        }

        // concatenate all manifest files into one, validate it and add it to the zip archive.
        zipArchive->AddManifestFile(AggregateManifestsAndValidate(manifests));
      }
      // Add resource files to the zip archive
      for (option::Option* resopt = options[RESADD]; resopt;
           resopt = resopt->next()) {
        zipArchive->AddResourceFile(resopt->arg);
      }
      // Merge resources from supplied zip archives
      for (option::Option* opt = options[ZIPADD]; opt; opt = opt->next()) {
        zipArchive->AddResourcesFromArchive(opt->arg);
      }
    }
    // ---------------------------------------------------------------------------------
    //      APPEND ZIP to BINARY if bundle-file is specified
    // ---------------------------------------------------------------------------------
    if (bundleFileOpt) {
      validateManifestsInArchive(zipFile);
      std::string bundleBinaryFile(bundleFileOpt->arg);
      std::ofstream outFileStream(
        bundleBinaryFile, std::ios::ate | std::ios::binary | std::ios::app);
      std::ifstream zipFileStream(zipFile, std::ios::in | std::ios::binary);
      if (outFileStream.is_open() && zipFileStream.is_open()) {
        std::clog << "Appending file " << bundleBinaryFile
                  << " with contents of resources zip file at " << zipFile
                  << std::endl;
        std::clog << "  Initial file size : " << outFileStream.tellp()
                  << std::endl;
        outFileStream << zipFileStream.rdbuf();
        std::clog << "  Final file size : " << outFileStream.tellp()
                  << std::endl;
        // Depending on the ofstream destructor to close the file may result in a silent
        // file write error. Hence the explicit call to close.
        outFileStream.close();
        if (outFileStream.rdstate() & std::ofstream::failbit) {
          std::cerr << "Failed to write file : " << bundleBinaryFile
                    << std::endl;
          return_code = EXIT_FAILURE;
        }
      } else {
        std::cerr << "Opening file "
                  << (outFileStream.is_open() ? zipFile : bundleBinaryFile)
                  << " failed" << std::endl;
        return_code = EXIT_FAILURE;
      }
    }
  } catch (const InvalidManifest& ex) {
    std::cerr << "JSON Parsing Error: " << ex.what() << std::endl;
    return_code = BUNDLE_MANIFEST_VALIDATION_ERROR_CODE;
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return_code = EXIT_FAILURE;
  }

  // delete temporary file and report error on failure
  if (deleteTempFile && (std::remove(zipFile.c_str()) != 0)) {
    std::cerr << "Error removing temporary zip archive " << zipFile
              << std::endl;
    return_code = EXIT_FAILURE;
  }

  // clear the failbit set by us
  if (!options[VERBOSE]) {
    std::clog.clear();
  }

  return return_code;
}
