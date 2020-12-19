#pragma once

#include "bitboard.h"
#include "endgame.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"




class Runner
{

public:
	void init();
	void posEstimate(int argc, char* argv[]);
};

