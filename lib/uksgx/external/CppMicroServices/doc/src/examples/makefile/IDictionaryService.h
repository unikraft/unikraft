#ifndef IDICTIONARYSERVICE_H
#define IDICTIONARYSERVICE_H

#include "cppmicroservices/ServiceInterface.h"

#include <string>

#ifdef MODULE_EXPORTS
#  define MODULE_EXPORT US_ABI_EXPORT
#else
#  define MODULE_EXPORT US_ABI_IMPORT
#endif

/**
 * A simple service interface that defines a dictionary service.
 * A dictionary service simply verifies the existence of a word.
 **/
struct MODULE_EXPORT IDictionaryService
{
  virtual ~IDictionaryService();

  /**
   * Check for the existence of a word.
   * @param word the word to be checked.
   * @return true if the word is in the dictionary,
   *         false otherwise.
   **/
  virtual bool CheckWord(const std::string& word) = 0;
};
#endif // DICTIONARYSERVICE_H
