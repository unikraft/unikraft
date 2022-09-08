set(_srcs Activator.cpp ISpellCheckService.cpp)

set(spellcheckservice_DEPENDS dictionaryservice)
CreateTutorial(spellcheckservice ${_srcs})
