#pragma once

#include "bitboard.h"
#include "endgame.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"
#include "OpeningLimit.h"
#include "LegalChecker.h"


class Runner
{
private:
	std::vector<OpeningLimit> openingsToCheck;

public:
	void saveFile(std::string fname, const std::vector<std::string>& v);
	void init();
	void generateFens(int argc, char* argv[], std::string fname);
	template<ESampleType sampleType>
	void posEstimate(int argc, char* argv[]);
};

