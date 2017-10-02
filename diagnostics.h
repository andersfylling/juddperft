#ifndef _DIAGNOSTICS_H
#define _DIAGNOSTICS_H 1
#include "movegen.h"

#include <string>

#define INCLUDE_DIAGNOSTICS 1
#define PERFTVALIDATE_TRUE 1
#define PERFTVALIDATE_FALSE 0
#define PERFTVALIDATE_FAILEDTOSTART -1

#ifdef INCLUDE_DIAGNOSTICS

// PerftValidateWithExternal() - validates perft calculation against external engine
int PerftValidateWithExternal(const std::string& validatorPath, const std::string& fenString, int depth, int64_t value);
void FindPerftBug(const std::string& validatorPath, const ChessPosition* pP, int depth);
void RunTestSuite();
void DumpPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, uint64_t correctAnswer);

#endif // INCLUDE_DIAGNOSTICS
#endif //  _DIAGNOSTICS_H

