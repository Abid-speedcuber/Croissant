/*
 * SQUARE-1 OPTIMISER version 2.1
 * by Jaap Scherphuis, jaapsch@yahoo.com, copyright 2003-2011
 * and Michael Gottlieb, qqwref@gmail.com, copyright 2023-2024
 */

#include <fstream>
#include <iostream>
#include <cstring>
#include <ctime>
#include <vector>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <stdexcept>

#define NUMHALVES 13
#define NUMLAYERS 158
#define NUMSHAPES 7356
#define FILESTT "sq1stt.dat"
#define FILESCTE "sq1scte.dat"
#define FILESCTC "sq1sctc.dat"
#define FILEP1U  "sq1p1u.dat"
#define FILEP2U  "sq1p2u.dat"
#define FILEP1W  "sq1p1w.dat"
#define FILEP2W  "sq1p2w.dat"
#define FILEP1A  "sq1p1a.dat"
#define FILEP2A  "sq1p2a.dat"

#define TURN_METRIC 0
#define SLICE_METRIC 1
#define ANGLE_METRIC 2

const char* errors[]={
	"Unrecognised command line switch.", //1
	"Too many command line arguments.",
	"Input file not found.",//3
	"Bracket ) expected.",//4
	"Bottom layer turn expected.",//5
	"Comma expected.",//6
	"Top layer turn expected.",//7
	"Bracket ( expected.",//"8
	"Position should be 16 or 17 characters.",//9
	"Expected A-H or 1-8.",//10
	"Expected - or /.",//11
	"Slice is blocked by corner.",//12
	"Can't parse input as position string or movelist.",//13
	"Unexpected bracket (.",//14
	"Number expected.",//15
	"Slice / expected.",//16
	"Position string has too many copies of a piece.",//17
	"Can't stay in cube shape and also use 2gen.",//18
	"Position can't be solved with these constraints",//19
};

#include "karnotation.h"
#include "sq1-logic.h"

int verbosity = 5;
bool generator=false;
bool usenegative=false;
bool usebrackets=false;
bool karnotation=false;
bool specificAngleTop=false;
bool specificAngleBot=false;
int metric = TURN_METRIC;
// 0=both  1=preABF  2=postABF  3=none (default — matches old solver-mode behaviour of
// normalizing postABF, but the UI default is "none" so the user opts in explicitly)
int maxX = 6;
int maxY = 6;
int maxTotal = 12;
std::vector<int> specificDepths;

static std::string tableDirectory = ".";
static std::atomic_bool stopRequested{false};
static bool s_hasInjectedPosition = false;
static int  s_injectedPos[24];
static int  s_injectedMiddle = 1;
static bool g_extendedOutput = false;

void sq1optSetExtendedOutput(bool val) { g_extendedOutput = val; }

void sq1optSetTableDirectory(const std::string& dir)
{
	tableDirectory = dir.empty() ? "." : dir;
}

void sq1optRequestStop()
{
	stopRequested.store(true);
}

void sq1optSetPosition(const int pos[24], int middle)
{
	for (int i = 0; i < 24; i++) s_injectedPos[i] = pos[i];
	s_injectedMiddle = middle;
	s_hasInjectedPosition = true;
}

// Stable C ABI entry points used by the browser build.  Keeping these wrappers
// here means the WASM proof of concept calls the same solver as the desktop app.
extern "C" {
void sq1opt_web_set_table_directory(const char* dir)
{
	 sq1optSetTableDirectory(dir ? std::string(dir) : std::string());
}

void sq1opt_web_request_stop()
{
	sq1optRequestStop();
}
}

static std::string tablePath(const char* fileName)
{
	if (tableDirectory == "." || tableDirectory.empty()) return fileName;
	const char last = tableDirectory[tableDirectory.size() - 1];
	if (last == '/' || last == '\\') return tableDirectory + fileName;
	return tableDirectory + "/" + fileName;
}

static void resetSolverOptions()
{
	stopRequested.store(false);
	verbosity = 5;
	generator = false;
	usenegative = false;
	usebrackets = false;
	karnotation = false;
	specificAngleTop = false;
	specificAngleBot = false;
	metric = TURN_METRIC;
	maxX = 6;
	maxY = 6;
	maxTotal = 12;
	specificDepths.clear();
}

static inline void throwIfStopped()
{
	if (stopRequested.load()) throw std::runtime_error("Solver stopped.");
}

class HalfLayer {
public:
	int pieces, turn, nPieces;
	HalfLayer(int p, int t) {
		int nEdges=0;
		pieces = p;
		for(int i=0, m=1; i<6; i++, m<<=1){
			if( (pieces&m)!=0 ) nEdges++;
		}
		nPieces=3+nEdges/2;
		turn=t;
	}
};

class Layer {
public:
	HalfLayer& h1, & h2;
	int turnt, turnb;
	int nPieces;
	bool turnParityOdd;
	bool turnParityOddb;
	int pieces;
	int tpieces, bpieces;   // result after turn

	Layer( HalfLayer& p1, HalfLayer& p2): h1(p1), h2(p2) {
		pieces = (h1.pieces<<6)+h2.pieces;
		nPieces = h1.nPieces + h2.nPieces;

		int m=1;
		for(turnt=1; turnt<6; turnt++){
			if( (h1.turn&h2.turn&m)!=0 ) break;
			m<<=1;
		}
		if( turnt==6 ) turnb=6;
		else{
			m=1<<4;
			for(turnb=1; turnb<5; turnb++){
				if( (h1.turn&h2.turn&m)!=0 ) break;
				m>>=1;
			}
		}

		tpieces=pieces;
		int nEdges=0;
		for( int i=0; i<turnt; i++ ){
			if( (tpieces&1)!=0 ) { tpieces+=(1<<12); nEdges++; }
			tpieces>>=1;
		}
		//find out parity of that layer turn
		// Is odd cycle if even # pieces, and odd number passes seam
		//  Note (turn+edges)/2 = number of pieces crossing seam
		turnParityOdd = (nPieces&1)==0 && ((turnt+nEdges)&2)!=0;

		bpieces=pieces;
		nEdges=0;
		for( int i=0; i<turnb; i++ ){
			bpieces<<=1;
			if( (bpieces&(1<<12))!=0 ) { bpieces-=(1<<12)-1; nEdges++; }
		}
		//find out parity of that layer turn
		// Is odd cycle if even # pieces, and odd number passes seam
		//  Note (turn+edges)/2 = number of pieces crossing seam
		turnParityOddb = (nPieces&1)==0 && ((turnb+nEdges)&2)!=0;

	}
};

class Sq1Shape {
public:
	Layer& topl, &botl;
	int pieces;
	bool parityOdd;
	int tpieces[4];
	bool tparity[4];
	Sq1Shape( Layer& l1, Layer& l2, bool p) : topl(l1), botl(l2) {
		parityOdd=p;
		pieces = (l1.pieces<<12)+l2.pieces;
		tpieces[0] = (l1.tpieces<<12)+l2. pieces;
		tpieces[1] = (l1. pieces<<12)+l2.bpieces;
		tpieces[2] = (l1.h1.pieces<<18)+(l2.h1.pieces<<12)+(l1.h2.pieces<<6)+(l2.h2.pieces);
		// calculate mirrored shape
		tpieces[3] = 0;
		for( int m=1, i=0; i<24; i++,m<<=1){
			tpieces[3]<<=1;
			if( (pieces&m)!=0 ) tpieces[3]++;
		}
		tparity[0] = parityOdd^l1.turnParityOdd;
		tparity[1] = parityOdd^l2.turnParityOddb;
		tparity[2] = parityOdd^( (l1.h2.nPieces&1)!=0 && (l2.h1.nPieces&1)!=0 );
		tparity[3] = parityOdd;
	}
};


class ChoiceTable {
public:
	unsigned char choice2Idx[256];
	unsigned char idx2Choice[70];
	ChoiceTable(){
		unsigned char nc=0;
		for( int i=0; i<255; i++ ) choice2Idx[i]=255;
		for( int i=1; i<255; i<<=1 ){
			for( int j=i+i; j<255; j<<=1 ){
				for( int k=j+j; k<255; k<<=1 ){
					for( int l=k+k; l<255; l<<=1 ){
						choice2Idx[i+j+k+l]=nc;
						idx2Choice[nc++]=(unsigned char)(i+j+k+l);
					}
				}
			}
		}
	}
};


class ShapeTranTable {
public:
	int nShape;
	Sq1Shape* shapeList[NUMSHAPES];
	int (*tranTable)[4];
	HalfLayer* hl[NUMHALVES];
	Layer* ll[NUMLAYERS];

	ShapeTranTable(){
		//first build list of possible halflayers
		int hi[]={ 0,    3,12,48, 9,36,33,  15,39,51,57,60,  63};
		int ht[]={42,   43,46,58,45,54,53,  47,55,59,61,62,  63};
		for( int i=0; i<NUMHALVES; i++ ){ hl[i]=new HalfLayer(hi[i],ht[i]); }

		//Now build list of possible Layers
		int lll=0;
		for( int i=0; i<NUMHALVES; i++ ){
			for( int j=0; j<NUMHALVES; j++ ){
				if( hl[i]->nPieces + hl[j]->nPieces<=10 ){
					ll[lll++]=new Layer( *hl[i], *hl[j] );
				}
			}
		}

		//Now build list of all possible shapes
		nShape=0;
		for( int i=0; i<lll; i++ ){
			for( int j=0; j<lll; j++ ){
				if( ll[i]->nPieces + ll[j]->nPieces==16 ){
					shapeList[nShape++]=new Sq1Shape( *ll[i], *ll[j], true );
					shapeList[nShape++]=new Sq1Shape( *ll[i], *ll[j], false );
				}
			}
		}

		// At last we can calculate full transition table
		tranTable = new int[NUMSHAPES][4];
		// see if can be found on file
		std::ifstream is(tablePath(FILESTT), std::ios::binary);
		if( is.fail() ){
			// no file. calculate table.
			for( int i=0; i<nShape; i++ ){
				throwIfStopped();
				//effect on shape of each move, incuding reflection
				for( int m=0; m<4; m++ ){
					for( int j=0; j<nShape; j++ ){
						if( shapeList[i]->tpieces[m] == shapeList[j]->pieces &&
							shapeList[i]->tparity[m] == shapeList[j]->parityOdd ){
							tranTable[i][m]=j;
							break;
						}
					}
				}
			}
			// save to file
			std::ofstream os(tablePath(FILESTT), std::ios::binary);
			os.write( (char*)tranTable, nShape*4*sizeof(int) );
		}else{
			// read from file
			nShape = NUMSHAPES;
			is.read( (char*)tranTable, nShape*4*sizeof(int) );
		}
	}
	~ShapeTranTable(){
		for( int i=0; i<NUMHALVES; i++ ){ delete hl[i]; }
		for( int i=0; i<NUMLAYERS; i++ ){ delete ll[i]; }
		for( int i=0; i<nShape; i++ ){ delete shapeList[i]; }
		delete[] tranTable;
	}
	inline int getShape(int s, bool p){
		for( int i=0; i<nShape; i++){
			if( shapeList[i]->pieces == s && shapeList[i]->parityOdd==p ) return i;
		}
		return -1;
	}
	inline int getTopTurn(int s){
		return shapeList[s]->topl.turnt;
	}
	inline int getBotTurn(int s){
		return shapeList[s]->botl.turnb;
	}
};

class ShapeColPos {
	ShapeTranTable &stt;
	ChoiceTable &ct;
	int shapeIx;
	int colouring; //24bit string
	bool edgesFlag;
public:
	ShapeColPos( ShapeTranTable& stt0, ChoiceTable& ct0)
		: stt(stt0), ct(ct0) {}
	void set( int shp, int col, bool edges )
	{
		// col is 8 bit colouring of one type of piece.
		// edges set then edge colouring, else corner colouring
		// get full 24 bit colouring.
		int c=ct.idx2Choice[col];
		shapeIx = shp;
		edgesFlag = edges;
		colouring=0;
		int s=stt.shapeList[shapeIx]->pieces;
		if( edges ){
			for( int m=1, i=0, n=1; i<24; m<<=1, i++){
				if( (s&m)!=0 ) {
					if( (c&n)!=0 ) colouring |= m;
					n<<=1;
				}
			}
		}else{
			for( int m=3, i=0, n=1; i<24; m<<=1, i++){
				if( (s&m)==0 ) {
					if( (c&n)!=0 ) colouring |= m;
					n<<=1;
					m<<=1; i++;
				}
			}
		}
	}
	void domove(int m){
		const int botmask = (1<<12)-1;
		const int topmask = (1<<24)-(1<<12);
		const int botrmask = (1<<12)-(1<<6);
		const int toprmask = (1<<18)-(1<<12);
		const int leftmask = botmask+topmask-botrmask-toprmask;
		if( m==0 ){
			int tn=stt.getTopTurn(shapeIx);
			int b=colouring&botmask;
			int t=colouring&topmask;
			t+=(t>>12);
			t<<=(12-tn);
			colouring = b + (t&topmask);
		}else if( m==1 ){
			int tn=stt.getBotTurn(shapeIx);
			int b=colouring&botmask;
			int t=colouring&topmask;
			b+=(b<<12);
			b>>=(12-tn);
			colouring = t + (b&botmask);
		}else if( m==2 ){
			int b=colouring&botrmask;
			int t=colouring&toprmask;
			colouring = (colouring&leftmask) + (t>>6) + (b<<6);
		}
		shapeIx=stt.tranTable[shapeIx][m];
	}
	unsigned char getColIdx(){
		int c=0,n=1;
		int s=stt.shapeList[shapeIx]->pieces;
		if( edgesFlag ){
			for( int m=1, i=0; i<24; m<<=1, i++){
				if( (s&m)!=0 ) {
					if( (colouring&m)!=0 ) c |= n;
					n<<=1;
				}
			}
		}else{
			for( int m=3, i=0; i<24; m<<=1, i++){
				if( (s&m)==0 ) {
					if( (colouring&m)!=0 ) c |= n;
					n<<=1;
					m<<=1; i++;
				}
			}
		}
		return(ct.choice2Idx[c]);
	}
};



class ShpColTranTable {
public:
	char (*tranTable)[70][3];
	ShapeTranTable& stt;
	ChoiceTable& ct;

	ShpColTranTable( ShapeTranTable& stt0, ChoiceTable& ct0, bool edges )
		: stt(stt0), ct(ct0)
	{
		ShapeColPos p(stt,ct);
		tranTable = new char[NUMSHAPES][70][3];

		// see if can be found on file
		std::ifstream is(tablePath(edges? FILESCTE : FILESCTC), std::ios::binary);
		if( is.fail() ){
			// no file. calculate table.
			// Calculate transition table
			int i,j,m;
			for( m=0; m<3; m++ ){
				for( i=0; i<NUMSHAPES; i++ ){
					throwIfStopped();
					for( j=0; j<70; j++){
						p.set(i,j,edges);
						p.domove(m);
						tranTable[i][j][m]=p.getColIdx();
						if( p.getColIdx()==255 ){
							throw std::runtime_error("Invalid shape/color transition table entry.");
						}
					}
				}
			}
			// save to file
			std::ofstream os(tablePath(edges? FILESCTE : FILESCTC), std::ios::binary);
			os.write( (char*)tranTable, NUMSHAPES*3*70*sizeof(char) );
		}else{
			// read from file
			is.read( (char*)tranTable, NUMSHAPES*3*70*sizeof(char) );
		}
	}
	~ShpColTranTable(){
		delete[] tranTable;
	}
};

// FullPosition holds position with each piece individually specified.
// Pieces 0-7 are corners and appear twice in a row. Pieces 8-15 are edges and appear once
// Returns the valid preadf D rotations (doBot amounts) for 2-gen / pseudo-2-gen.
//
// A preadf is a doBot() amount k that rotates a solved block into the "frozen"
// bottom-left region (pos[18..23], i.e. offsets 6..11 after the rotation) so the
// remainder can be solved without further D moves between the first and last
// slice.  The post-last-slice D (postabf) that re-homes the globally-rotated D
// layer is NOT computed here — the solver discovers it as a real move via the
// slice-point check.
//
// For twoGen==2: any of the 8 contiguous 6-slot windows of the solved D layer
//   (M·E·N·F·O·G·P·H) must land in bottom-left.  D-right is frozen.
// For twoGen==1: any of the 4 solved CEC blocks can land at offsets 7..11 or
//   6..10; the extra ±1 D allowed between slices covers the wiggle.
// For twoGen==0: returns {0} (no preadf).
//
// After doBot(k): new pos[12+i] == old pos[12 + (i−k+12)%12].
//
// Single source of truth shared by FullPosition::findPreadf (solver) and the
// UI's Solve-button enable check; declared in sq1opt-runner.h.
// Whether a position value (concrete 0-15, or partially-specified: corner <0,
// edge >15, with value%3 selecting up/down/any) could represent the concrete
// piece `target` (0-15).  Mirror of FullPosition::singleMatch as a free function.
bool couldBe(int posVal, int target) {
	if (posVal == target) return true;
	if (posVal>15 && posVal%3==0  && target>=8  && target<=11) return true; // edge up (X)
	if (posVal>15 && posVal%3==1  && target>=12 && target<=15) return true; // edge down (Y)
	if (posVal<0  && posVal%3==0  && target>=0  && target<=3)  return true; // corner up (U)
	if (posVal<0  && posVal%3==-2 && target>=4  && target<=7)  return true; // corner down (V)
	if (posVal>15 && posVal%3==2  && target>=8  && target<=15) return true; // edge any (Z)
	if (posVal<0  && posVal%3==-1 && target>=0  && target<=7)  return true; // corner any (W)
	return false;
}

// Piece-count validity of a pos[24] array — the same rules enforced by
// Sq1Widget::setPositionFromString and FullPosition::parseInput: every concrete
// piece (0-15) appears at most once, and no layer holds more than 4 corners or 4
// edges of its type (8 total of each).  Side-effect free; corners occupy two
// adjacent slots.  Assumes a well-formed array (corner halves adjacent), which
// holds for any real position and for fills that don't split a corner across the
// 23/12 boundary (such fills produce a duplicate and are rejected here anyway).
bool validPosition(const int pos[24]) {
	int pieceCount[16] = {0};
	int cUp=0, cDown=0, cTot=0, eUp=0, eDown=0, eTot=0;
	for (int i=0; i<24; i++) {
		int k = pos[i];
		if (k>=0 && k<=15) { if (++pieceCount[k] > 1) return false; }
		if (k<8) { // corner (concrete 0-7 or partial <0)
			cTot++;
			if ((k<0 && k%3==0)  || (k>=0 && k<=3)) cUp++;
			if ((k<0 && k%3==-2) || (k>=4 && k<=7)) cDown++;
			i++; // corners occupy two slots
		} else {   // edge (concrete 8-15 or partial >15)
			eTot++;
			if ((k>15 && k%3==0) || (k>=8  && k<=11)) eUp++;
			if ((k>15 && k%3==1) || (k>=12 && k<=15)) eDown++;
		}
	}
	if (cUp>4 || cDown>4 || cTot>8 || eUp>4 || eDown>4 || eTot>8) return false;
	return true;
}

std::vector<int> twoGenPreadf(const int pos[24], int twoGen, bool firstMatchOnly = false) {
	std::vector<int> result;
	if (twoGen == 0) { result.push_back(0); return result; }

	// The 8 contiguous 6-slot windows of the solved D layer (2-gen blocks)...
	static const int blocks2g[8][6] = {
		{14, 6, 6,15, 7, 7}, // O·G·P·H (solved bottom-left)
		{15, 7, 7,12, 4, 4}, // P·H·M·E
		{12, 4, 4,13, 5, 5}, // M·E·N·F
		{13, 5, 5,14, 6, 6}, // N·F·O·G
		{ 6, 6,15, 7, 7,12}, // G·P·H·M
		{ 7, 7,12, 4, 4,13}, // H·M·E·N
		{ 4, 4,13, 5, 5,14}, // E·N·F·O
		{ 5, 5,14, 6, 6,15}, // F·O·G·P
	};
	// ...and the 4 solved CEC blocks (pseudo-2-gen).
	static const int blocksP2g[4][5] = {
		{4,4,13,5,5}, // E,6,F
		{5,5,14,6,6}, // F,7,G
		{6,6,15,7,7}, // G,8,H
		{7,7,12,4,4}, // H,5,E
	};

	// For each candidate rotation k, the bottom-left "frozen" region is at offsets
	// 6..11 (2-gen) — or 7..11 / 6..10 for the shorter p2g CEC block, since D±1 is
	// allowed.  realIdx(i) is the pos[] index that lands at bottom offset i after
	// doBot(k).  A candidate is valid if some block W is piece-by-piece compatible
	// with it (couldBe) AND writing W's concrete pieces there leaves a valid
	// position (no piece used twice / no layer overfull).  This handles fully
	// concrete and partially-specified positions uniformly.
	for (int k = 0; k < 12; k++) {
		auto realIdx = [&](int i) { return 12 + (i - k + 12) % 12; };
		bool ok = false;

		if (twoGen == 2) {
			for (const auto& W : blocks2g) {
				bool match = true;
				for (int j=0; j<6; j++) if (!couldBe(pos[realIdx(6+j)], W[j])) { match=false; break; }
				if (!match) continue;
				int copy[24]; for (int i=0;i<24;i++) copy[i]=pos[i];
				for (int j=0; j<6; j++) copy[realIdx(6+j)] = W[j];
				if (validPosition(copy)) { ok=true; break; }
			}
		} else {
			for (const auto& W : blocksP2g) {
				for (int base : {7, 6}) {
					bool match = true;
					for (int j=0; j<5; j++) if (!couldBe(pos[realIdx(base+j)], W[j])) { match=false; break; }
					if (!match) continue;
					int copy[24]; for (int i=0;i<24;i++) copy[i]=pos[i];
					for (int j=0; j<5; j++) copy[realIdx(base+j)] = W[j];
					if (validPosition(copy)) { ok=true; break; }
				}
				if (ok) break;
			}
		}
		if (ok) {
			result.push_back(k);
			// The UI only needs to know whether ANY preadf exists; the solver needs
			// the full set.  firstMatchOnly lets the UI bail out on the first hit.
			if (firstMatchOnly) return result;
		}
	}
	return result;
}

// Whether the 8 corners can be solved using only pseudo-2-gen moves (top turns +
// slices + D±1), i.e. the corner permutation is reachable in the 2-gen corner
// group.  Reads the corners from pos[0..17] (top layer + bottom-right).  Used by
// the solver's keep-cube-shape p2g guard and the UI's Solve-button enable check.
// Single source of truth; declared in sq1opt-runner.h.
bool has2GenCorners(const int pos[24]) {
	// get corners
	int tmp[6];
	int j=0;
	for (int i=0; i<18; i++) {
		if (pos[i]<8) {
			if (j%2 == 0) tmp[j/2] = pos[i];
			j++;
		}
	}
	// place D corners - if we find a D corner on U, AUF and then insert
	int found_d = -1;
	for (int i=0; i<4; i++) if(tmp[i]>3) found_d = i;
	if (found_d > -1) {
		int tmp2[4];
		for (int i=0; i<4; i++) tmp2[i] = tmp[i];
		for (int i=0; i<4; i++) tmp[i] = tmp2[(i + found_d) % 4];
		int k = tmp[0]; tmp[0] = tmp[4]; tmp[4] = k;
		k = tmp[2]; tmp[2] = tmp[3]; tmp[3] = k;
	}
	found_d = -1;
	for (int i=0; i<4; i++) if(tmp[i]>3) found_d = i;
	if (found_d > -1) {
		int tmp2[4];
		for (int i=0; i<4; i++) tmp2[i] = tmp[i];
		for (int i=0; i<4; i++) tmp[i] = tmp2[(i + found_d) % 4];
		int k = tmp[0]; tmp[0] = tmp[5]; tmp[5] = k;
		k = tmp[1]; tmp[1] = tmp[2]; tmp[2] = k;
	}
	// adjust if D corners are swapped, then AUF
	if (tmp[4] == 5 && tmp[5] == 4) {
		tmp[4] = 4; tmp[5] = 5;
		int k = tmp[0]; tmp[0] = tmp[2]; tmp[2] = k;
	}
	int found_u = -1;
	for (int i=0; i<4; i++) if(tmp[i]==0) found_u = i;
	if (found_u > -1) {
		int tmp2[4];
		for (int i=0; i<4; i++) tmp2[i] = tmp[i];
		for (int i=0; i<4; i++) tmp[i] = tmp2[(i + found_u) % 4];
	}
	if (tmp[0] == 0 && tmp[1] == 1 && tmp[2] == 2 && tmp[3] == 3 && tmp[4] == 4 && tmp[5] == 5) return true;
	return false;
}

// Partial-aware version of has2GenCorners.  Like has2GenCorners it assumes the two
// bottom-left corners are solved (G,H = 6,7) and works with the other 6 corners
// (pos[0..17]); the caller is responsible for placing a valid candidate block at
// bottom-left first.  Resolves partial corners by elimination *respecting each
// partial's layer constraint* (U=top corner 0-3, V=bottom corner 4-7, W=any),
// then defers to the concrete has2GenCorners:
//   * 0 partials   -> plain has2GenCorners.
//   * >=3 partials -> always solvable (enough free corners): true.
//   * 1-2 partials -> the missing corners of {0..5} are the candidates; try every
//                     assignment of them to the partial slots that is layer-valid
//                     (so two W's are interchangeable, but a U can't take a bottom
//                     corner, etc.) and accept if any resulting concrete state
//                     passes has2GenCorners.
bool partialHas2GenCorners(const int pos[24]) {
	int slot[6];          // first slot index of each of the 6 corners
	int ptype[6];         // partial type: 0=U(top), 1=V(bottom), 2=W(any), -1=concrete
	bool present[8] = {false}; // which concrete corner values appear among the 6
	int n = 0;
	for (int i = 0; i < 18 && n < 6; i++) {
		if (pos[i] < 8) { // corner (concrete 0-7 or partial <0)
			int v = pos[i];
			slot[n] = i;
			if (v < 0) ptype[n] = (v % 3 == 0) ? 0 : (v % 3 == -2 ? 1 : 2);
			else { ptype[n] = -1; present[v] = true; }
			n++;
			i++; // skip the duplicate corner slot
		}
	}

	int p[2], numPartial = 0;
	for (int t = 0; t < n; t++) if (ptype[t] >= 0) { if (numPartial < 2) p[numPartial] = t; numPartial++; }

	if (numPartial == 0) return has2GenCorners(pos);
	if (numPartial >= 3) return true;

	// Candidate corners for the partials: {0..5} (bottom-left holds 6,7) minus the
	// concrete corners already present among the 6.
	int avail[6], nA = 0;
	for (int c = 0; c < 6; c++) if (!present[c]) avail[nA++] = c;
	if (nA != numPartial) return false; // inconsistent input

	// A corner value is compatible with a partial slot iff it lies in the slot's
	// allowed layer: U -> 0-3, V -> 4-7, W -> anything.
	auto compat = [](int type, int val) {
		if (type == 0) return val >= 0 && val <= 3;
		if (type == 1) return val >= 4 && val <= 7;
		return true;
	};
	auto tryAssign = [&](int v0, int v1) -> bool {
		if (!compat(ptype[p[0]], v0)) return false;
		if (numPartial == 2 && !compat(ptype[p[1]], v1)) return false;
		int copy[24]; for (int i = 0; i < 24; i++) copy[i] = pos[i];
		copy[slot[p[0]]] = v0; copy[slot[p[0]] + 1] = v0;
		if (numPartial == 2) { copy[slot[p[1]]] = v1; copy[slot[p[1]] + 1] = v1; }
		return has2GenCorners(copy);
	};

	if (numPartial == 1) return tryAssign(avail[0], 0);
	// numPartial == 2: try both layer-valid assignments of the two candidate corners.
	return tryAssign(avail[0], avail[1]) || tryAssign(avail[1], avail[0]);
}

// Are the corners 2-gen-solvable for this position, evaluated once per valid preadf
// candidate?  For each preadf rotation k, doBot(k) brings a solved block to the
// frozen bottom-left.  has2GenCorners / partialHas2GenCorners assume the bottom-left
// pair is the canonical G,H with a canonical solved target, so we first relabel the
// corners by sigma — the map that sends doBot(k)*canonical back to canonical
// (identity on the top corners, a cyclic shift on the D-layer corners).  Because a
// value relabel commutes with the position permutations a 2-gen solve applies,
// checking the relabelled, rotated position against the canonical frame is exactly
// checking the original against doBot(k)*canonical.  Partial pieces carry only a
// layer constraint, which the shift preserves, so they pass through untouched.
// The position is corner-2-gen-solvable iff ANY candidate passes.  twoGen==0 -> true.
bool cornersAre2GenSolvable(const int pos[24], int twoGen) {
	if (twoGen == 0) return true;
	static const int C[24] = {0,0,8,1,1,9,2,2,10,3,3,11,12,4,4,13,5,5,14,6,6,15,7,7};
	auto doBotArr = [](int a[24], int m){
		m = ((m % 12) + 12) % 12;
		while (m-- > 0) { int c = a[23]; for (int i=23;i>12;i--) a[i]=a[i-1]; a[12]=c; }
	};
	for (int k : twoGenPreadf(pos, twoGen)) {
		int copy[24]; for (int i=0;i<24;i++) copy[i]=pos[i]; doBotArr(copy, k);
		int cano; // the amount to color shift by
		if (copy[23] >= 0 && copy[23] < 8) cano = (7 - copy[23]) * 3; // corner
		else cano = (7 - copy[22]) * 3; // last piece was an edge, take [22] instead
		int Ck[24]; for (int i=0;i<24;i++) Ck[i]=C[i]; doBotArr(Ck, cano);
		int sigma[8]; for (int i=0;i<8;i++) sigma[i]=i;
		for (int i=0;i<24;i++) if (Ck[i] >= 0 && Ck[i] < 8) sigma[Ck[i]] = C[i];
		for (int i=0;i<24;i++) if (copy[i] >= 0 && copy[i] < 8) copy[i] = sigma[copy[i]];
		if (partialHas2GenCorners(copy)) return true;
	}
	return false;
}

// Piece numbers below 0 are partially specified corners. Based on the value modulo 3, it's a
//  top corner (0), bottom corner (-2), or any corner (-1).
// Piece numbers above 15 are partially specified edges. Based on the value modulo 3, it's
//  top edge (0), bottom edge (1), or any edge (2).
class FullPosition {
public:
	int pos[24];
	int middle;
	FullPosition(){ reset(); }
	void reset(){
		middle=1;
		for( int i=0; i<24; i++)
			pos[i]="AAIBBJCCKDDLMEENFFOGGPHH"[i]-'A';
	}
	void print(){
		for(int i=0; i<24; i++){
			if (pos[i] < 0) {
				std::cout<<"UWV"[(-pos[i])%3];
			} else if (pos[i] > 15) {
				std::cout<<"XYZ"[pos[i]%3];
			} else {
				std::cout<<"ABCDEFGH12345678"[pos[i]];
			}
			if( pos[i]<8 ) i++;
		}
		std::cout<<"/ -"[middle+1];
	}
	void random(int twoGen, bool keepCubeShape){
		middle = (rand()&1)!=0?-1:1;
		do{
			//make starting position
			int tmp[16];
			for( int i=0; i<8; i++) {
				tmp[2*i + (i>3?1:0)] = i;
				tmp[2*i + (i>3?0:1)] = 8+i;
			}
			// shuffle
			if (keepCubeShape) {
				bool parity = false;
				int cornersToMix = twoGen==1 ? 6 : 8;
				int edgesToMix = twoGen==1 ? 7 : 8;
				for (int i=0; i<cornersToMix; i++) {
					int j = i + rand() % (cornersToMix - i);
					int k = tmp[2*i + (i>3?1:0)];
					tmp[2*i + (i>3?1:0)] = tmp[2*j + (j>3?1:0)];
					tmp[2*j + (j>3?1:0)] = k;
					if (i!=j) parity ^= true;
				}
				for (int i=0; i<edgesToMix; i++) {
					int j = i + rand() % (edgesToMix - i);
					int k = tmp[2*i + (i>3?0:1)];
					tmp[2*i + (i>3?0:1)] = tmp[2*j + (j>3?0:1)];
					tmp[2*j + (j>3?0:1)] = k;
					if (i!=j) parity ^= true;
				}
				if (parity) {
					int k = tmp[0];
					tmp[0] = tmp[2];
					tmp[2] = k;
				}
			} else {
				int nToMix = twoGen==2 ? 12 : (twoGen==1 ? 13 : 16);
				for( int i=0;i<nToMix; i++){
					int j=rand()%(nToMix-i);
					int k=tmp[i];tmp[i]=tmp[i+j];tmp[i+j]=k;
				}
			}
			//convert to position array
			for(int i=0, j=0;i<16;i++){
				pos[j++]=tmp[i];
				if( tmp[i]<8 ) pos[j++]=tmp[i];
			}
			// if p2g and keeping cubeshape, are the corners solvable in 2gen? if not, try again
			if (twoGen == 1 && keepCubeShape) {
				if (!has2GenCorners()) {
					pos[6] = pos[5]; continue; // fail the condition
				}
			}
			// ABF. if keeping cube shape, adjust both; otherwise, if p2g, adjust D only
			if (keepCubeShape) {
				if ((rand()&1)!=0) {
					tmp[0] = pos[11];
					for (int i=10; i>=0; i--) {
						pos[i+1] = pos[i];
					}
					pos[0] = tmp[0];
				}
				if ((rand()&1)!=0) {
					tmp[0] = pos[12];
					for (int i=12; i<=22; i++) {
						pos[i] = pos[i+1];
					}
					pos[23] = tmp[0];
				}
			} else if (twoGen == 1 && (rand()&1)!=0 && pos[11]!=pos[12]) {
				// in pseudo 2gen, with 50% chance, if the layers are validly separated, do a (0,-1)
				tmp[0] = pos[12];
				for (int i=12; i<=22; i++) {
					pos[i] = pos[i+1];
				}
				pos[23] = tmp[0];
			}
			// test sliceable
		}while( pos[5]==pos[6] || pos[11]==pos[12] || pos[17]==pos[18] || pos[12]==pos[23]);
	}
	void set(int p[],int m){
		for(int i=0;i<24;i++)pos[i]=p[i];
		middle=m;
	};
	void doTop(int m){
		m%=12;
		if(m<0)m+=12;
		while(m>0){
			int c=pos[11];
			for(int i=11;i>0;i--) pos[i]=pos[i-1];
			pos[0]=c;
			m--;
		}
	}
	void doBot(int m){
		m%=12;
		if(m<0)m+=12;
		while(m>0){
			int c=pos[23];
			for(int i=23;i>12;i--) pos[i]=pos[i-1];
			pos[12]=c;
			m--;
		}
	}
	bool doSlice(){
		if( !isSliceable() ) return false;
		for(int i=6;i<12;i++){
			int c=pos[i];
			pos[i]=pos[i+6];
			pos[i+6]=c;
		}
		middle=-middle;
		return true;
	}
	bool isSliceable(){
		return( pos[0]!=pos[11] && pos[5]!=pos[6] && pos[12]!=pos[23] && pos[17]!=pos[18] );
	}
	int getShape(){
		int s=0;
		for(int m=1<<23,i=0; i<24; i++,m>>=1){
			if(pos[i]>=8) s|=m;
		}
		return(s);
	}
	bool getParityOdd(){
		bool p=false;
		for(int i=0; i<24; i++){
			for(int j=i; j<24; j++){
				if( pos[j]<pos[i]) p=!p;
				if(pos[j]<8)j++;
			}
			if(pos[i]<8)i++;
		}
		return(p);
	}
	int getEdgeColouring(int cl){
		const int clp[3][4]={ { 8, 9,10,11}, { 8, 9,13,14}, {15,14,10, 9} };
		int c=0;
		int cnt=0;
		int m=(cl!=2)?1<<7:1;
		for(int i=0; i<24; i++){
			if( pos[i]>=8 ){
				for(int j=0; j<4; j++){
					if( pos[i]==clp[cl][j] || (pos[i]>15 && pos[i]%3==0 && cl==0)) { // edge up
						c|=m;
						cnt++;
						break;
					}
				}
				if(cl!=2) m>>=1; else m<<=1;
			}
		}
		if (cnt==4) return c;
		else return -1;
	}
	int getCornerColouring(int cl){
		const int clp[3][4]={ {0,1,2,3}, {0,1,5,6}, {7,6,2,1} };
		int c=0;
		int cnt=0;
		int m=(cl!=2)?1<<7:1;
		for(int i=0; i<24; i++){
			if( pos[i]<8 ){
				for(int j=0; j<4; j++){
					if( pos[i]==clp[cl][j] || (pos[i]<0 && pos[i]%3==0 && cl==0)) { // corner up
						c|=m;
						cnt++;
						break;
					}
				}
				if(cl!=2) m>>=1; else m<<=1;
				i++;
			}
		}
		if (cnt==4) return c;
		else return -1;
	}
	bool parseNumberForward(const char*inp, int& ix, int& num){
		bool min = false;
		num = 0;
		while( inp[ix]==' ' ) ix++;
		if( inp[ix]=='-') {
			min=true;
			ix++;
		}
		if( inp[ix]<'0' || inp[ix]>'9' ) return true;
		while( inp[ix]>='0' && inp[ix]<='9' ){
			num =num*10+(inp[ix]-'0');
			ix++;
		}
		if( min ) num = -num;
		while( inp[ix]==' ' ) ix++;
		return false;
	}
	bool parseNumberBackward(const char*inp, int& ix, int& num){
		int digvalue = 1;
		num = 0;
		while( ix>=0 && inp[ix]==' ' ) ix--;
		if( ix<0 ) return true;
		if( inp[ix]<'0' || inp[ix]>'9' ) return true;
		while( ix>=0 && inp[ix]>='0' && inp[ix]<='9' ){
			num =num+digvalue*(inp[ix]-'0');
			digvalue*=10;
			ix--;
		}
		if( ix>=0 && inp[ix]=='-'){
			num = -num;
			ix--;
		}
		while( ix>=0 && inp[ix]==' ' ) ix++;
		return false;
	}
	int parseInput( const char* inp ){
		// scan characters
		const char* t=inp;
		int f=0;
		while(*t){
			if( *t == ',' || *t == '(' || *t == ')' || *t == '9' || *t == '0' ){
				f|=1; // cannot be position string, but may be movelist
			}else if( (*t>='a' && *t<='h') || (*t>='A' && *t<='H') || (*t>='u' && *t<='z') || (*t>='U' && *t<='Z') ){
				f|=2; // cannot be movelist, but may be position string
			}else if( *t!='/' && *t!='-' && (*t<'1' || *t>'8') ){
				f|=3; // cannot be either
			}
			t++;
		}
		if( f==3 || f==0 ){
			return(13);
		}

		reset();
		int lw=0,lu=0;
		if( f==1 && !generator){
			// solution move sequence. start parsing from end
			int md=0;
			int i=strlen(inp)-1;
			while( i>=0 ){
				while( i>=0 && inp[i]==' ' ) i--;
				if( md==0 ){   // parsing any move
					if(inp[i]=='/') md = 1;
					else md = 2;
				}else if( md==1 ){
					if(inp[i--]!='/') return 16;
					if(!doSlice()) return 12;
					lu++;lw++;
					md=2;
				}else if( md==2 ){
					int m = 0;
					bool br=false;
					if( inp[i]==')' ) { i--; br=true; }
					// parsing bot turn
					if( parseNumberBackward(inp, i, m) ) return 5;
					m%=12;
					doBot(-m);
					if(m!=0) lu++;
					if( i<0 || inp[i--]!=',' ) return 6;
					// parsing top turn
					if( parseNumberBackward(inp, i, m) ) return 7;
					m%=12;
					doTop(-m);
					if(m!=0) lu++;
					if( br && ( i<0 || inp[i--]!='(' )) return 8;
					md--;
				}
			}
			if( !isSliceable() ) return 12;
			if( verbosity>=2) std::cout<<"Input:"<<inp<<" ["<<lw<<"|"<<lu<<"]"<<std::endl;
		}else if( f==1 ){
			// generating move sequence. start parsing from beginning
			int md=0;
			int i=0;
			while( inp[i]!=0 ){
				while( inp[i]==' ' ) i++;
				if( md==0 ){   // parsing any move
					if(inp[i]=='/') md = 1;
					else md = 2;
				}else if( md==1 ){
					if(inp[i++]!='/') return 16;
					if(!doSlice()) return 12;
					lu++;lw++;
					md=2;
				}else if( md==2 ){
					int m = 0;
					bool br=false;
					if( inp[i]=='(' ) { i++; br=true; }
					// parsing top turn
					if( parseNumberForward(inp, i, m) ) return 7;
					m%=12;
					doTop(m);
					if(m!=0) lu++;
					if( inp[i++]!=',' ) return 6;
					// parsing bot turn
					if( parseNumberForward(inp, i, m) ) return 5;
					m%=12;
					doBot(m);
					if(m!=0) lu++;
					if( br && inp[i++]!=')' ) return 4;
					md--;
				}
			}
			if( !isSliceable() ) return 12;
			if( verbosity>=2) std::cout<<"Input:"<<inp<<" ["<<lw<<"|"<<lu<<"]"<<std::endl;
		}else{
			// position
			if( strlen(inp)!=16 && strlen(inp)!=17 ) return(9);
			int pieceCount[16]; // track counts of each piece, so we can detect multiples of one piece
			int cecount[6]; // track total [up, down, all] + 3*[corners, edges]
			for (int i=0; i<16; i++) pieceCount[i] = 0;
			for (int i=0; i<6; i++) cecount[i] = 0;
			int j=0;
			int pi[24];
			// we can't reuse a piece number because two of the same number means a corner, so
			// each partially defined piece gets a separate set of 3 possible values
			int nextPartialCorner = -3;
			int nextPartialEdge = 18;
			for( int i=0; i<16; i++){
				int k=inp[i];
				if(k>='a' && k<='z') k+=('A'-'a');
				if(k>='A' && k<='H') k-='A';
				else if(k>='1' && k<='8') k-='1'-8;
				else if(k>='U' && k<='W') {
					k+=(nextPartialCorner-'U');
					nextPartialCorner -= 3;
				}
				else if(k>='X' && k<='Z') {
					k+=(nextPartialEdge-'X');
					nextPartialEdge += 3;
				}
				else return(10);
				pi[j++] = k;
				if (k>=0 && k<=15) pieceCount[k]++;
				if (k<8) {
					pi[j++] = k;
					cecount[2]++;
					if ((k<0 && k%3==0) || (k>=0 && k<=3)) cecount[0]++; // corner up
					if ((k<0 && k%3==-2) || (k>=4 && k<=7)) cecount[1]++; // corner down
				} else {
					cecount[5]++;
					if ((k>15 && k%3==0) || (k>=8 && k<=11)) cecount[3]++; // edge up
					if ((k>15 && k%3==1) || (k>=12 && k<=15)) cecount[4]++; // edge down
				}
			}
			for (int i=0; i<16; i++) {
				if (pieceCount[i] > 1) return(17);
			}
			if (cecount[0] > 4 || cecount[1] > 4 || cecount[2] > 8 || cecount[3] > 4 || cecount[4] > 4 || cecount[5] > 8) return 17;
			int midLayer=0;
			if( strlen(inp)==17 ){
				int k=inp[16];
				if( k!='-' && k!='/' ) return(11);
				midLayer = (k=='-') ? 1 : -1;
			}
			set(pi,midLayer);
		}
		return(0);
	}
	// assuming we're in a square/square shape, check if the corners are solvable with 2gen
	// Whether the corners can be solved with pseudo-2-gen moves — see has2GenCorners().
	bool has2GenCorners(){ return ::has2GenCorners(pos); }
	// Valid preadf D rotations for 2-gen / pseudo-2-gen — see twoGenPreadf().
	std::vector<int> findPreadf(int twoGen) const { return twoGenPreadf(pos, twoGen); }
	bool singleMatch(int posI, int solvedI) { return couldBe(posI, solvedI); }
	bool matchesSolved() {
		int solved[24] = {0, 0, 8, 1, 1, 9, 2, 2, 10, 3, 3, 11, 12, 4, 4, 13, 5, 5, 14, 6, 6, 15, 7, 7};
		for (int i=0; i<24; i++) {
			if (!singleMatch(pos[i], solved[i])) return false;
		}
		return true;
	}
	bool isPartial() {
		for (int i=0; i<24; i++) {
			if (pos[i] < 0 || pos[i] > 15) return true;
		}
		return false;
	}

};

//pruning table for combination of shape,edgecolouring,cornercolouring.
class PrunTable {
public:
	char (*table)[70][70];
	ShapeTranTable& stt;
	ShpColTranTable& scte;
	ShpColTranTable& sctc;

	PrunTable( FullPosition& p0, int cl, ShapeTranTable& stt0, ShpColTranTable& scte0, ShpColTranTable& sctc0)
		: stt(stt0), scte(scte0), sctc(sctc0)
	{
		// Calculate pruning table
		table = new char[NUMSHAPES][70][70];
		std::string fname;
		if(metric == TURN_METRIC){
			fname = tablePath((cl==0)? FILEP1U : FILEP2U);
		} else if (metric == ANGLE_METRIC) {
			fname = tablePath((cl==0)? FILEP1A : FILEP2A);
		} else {
			fname = tablePath((cl==0)? FILEP1W : FILEP2W);
		}

		// see if can be found on file
		std::ifstream is( fname, std::ios::binary );
		if( is.fail() ){
			// no file. calculate table.
			// clear table
			for( int i0=0; i0<NUMSHAPES; i0++ ){
			for( int i1=0; i1<70; i1++){
			for( int i2=0; i2<70; i2++){
				table[i0][i1][i2]=0;
			}}}
			//set start position
			int s0 = stt.getShape(p0.getShape(),p0.getParityOdd());
			int e0 = p0.getEdgeColouring(cl);
			int c0 = p0.getCornerColouring(cl);
			e0 = scte0.ct.choice2Idx[e0];
			c0 = sctc0.ct.choice2Idx[c0];
			if (metric == TURN_METRIC || metric == ANGLE_METRIC){
				table[s0][e0][c0]=1;
			}else{
				setAll(s0,e0,c0,1);
			}

			char l=1;
			int n=1;
			int last_nonzero=-1;
			do{
				throwIfStopped();
				if(verbosity>=6) std::cout<<" l="<<(int)(l-1)<<"  n="<<(int)n<<std::endl;
				n=0;
				if (metric == TURN_METRIC){
					for( int i0=0; i0<NUMSHAPES; i0++ ){
					for( int i1=0; i1<70; i1++){
					for( int i2=0; i2<70; i2++){
						if( table[i0][i1][i2]==l ){
							for( int m=0; m<3; m++){
								int j0=i0, j1=i1, j2=i2;
								int w=0;
								do{
									j2=sctc.tranTable[j0][j2][m];
									j1=scte.tranTable[j0][j1][m];
									j0=stt.tranTable[j0][m];
									if( table[j0][j1][j2]==0 ){
										table[j0][j1][j2]=l+1;
										n++;
									}
									w++;
									if(w>12){
										throw std::runtime_error("Invalid pruning table turn cycle.");
									}
								}while(j0!=i0 || j1!=i1 || j2!=i2 );
							}
						}
					}}}
				}else if (metric == ANGLE_METRIC) {
					for( int i0=0; i0<NUMSHAPES; i0++ ){
					for( int i1=0; i1<70; i1++){
					for( int i2=0; i2<70; i2++){
						if( table[i0][i1][i2]==l ){
							for( int m=0; m<3; m++){ // m is the move type, U/D/slice
								int j0=i0, j1=i1, j2=i2;
								int w=0, newcnt=0;
								do{
									if(m==0){
										w+=stt.getTopTurn(j0);
									}else if(m==1){
										w+=stt.getBotTurn(j0);
									}else{
										w++;
									}
									// w is the move amount
									j2=sctc.tranTable[j0][j2][m];
									j1=scte.tranTable[j0][j1][m];
									j0=stt.tranTable[j0][m];
									if (m==2) {
										newcnt = l + 1;
									} else {
										newcnt = l + (w>6 ? 12-w : w);
									}
									if( table[j0][j1][j2]==0 || table[j0][j1][j2] > newcnt ){
										table[j0][j1][j2]=newcnt;
										n++;
									}
									if(w>12){
										throw std::runtime_error("Invalid pruning table angle cycle.");
									}
								}while(j0!=i0 || j1!=i1 || j2!=i2 );
							}
						}
					}}}
				}else{
					for( int i0=0; i0<NUMSHAPES; i0++ ){
					for( int i1=0; i1<70; i1++){
					for( int i2=0; i2<70; i2++){
						if( table[i0][i1][i2]==l ){
							// do slice
							int j0=stt.tranTable[i0][2];
							int j1=scte.tranTable[i0][i1][2];
							int j2=sctc.tranTable[i0][i2][2];
							if( table[j0][j1][j2]==0 ){
								n+=setAll(j0,j1,j2,l+1);
							}
						}
					}}}
				}
				l++;
				if (n!=0) last_nonzero=l;
			}while(l - last_nonzero < 10);
			if(verbosity>=6) std::cout<<std::endl;

		// save to file
			std::ofstream os( fname, std::ios::binary );
			os.write( (char*)table, NUMSHAPES*70*70*sizeof(char) );
		}else{
			// read from file
			is.read( (char*)table, NUMSHAPES*70*70*sizeof(char) );
		}


	}
	~PrunTable(){
		delete[] table;
	}
	// set a position to depth l, as well as all rotations of it.
	inline int setAll(int i0,int i1,int i2, char l){
		int n=0;
		int j0=i0, j1=i1, j2=i2;
		do{
			int k0=j0, k1=j1, k2=j2;
			do{
				if( table[k0][k1][k2]==0 ){
					table[k0][k1][k2]=l;
					n++;
				}
				k2=sctc.tranTable[k0][k2][0];
				k1=scte.tranTable[k0][k1][0];
				k0=stt.tranTable[k0][0];
			}while(j0!=k0 || j1!=k1 || j2!=k2 );
			j2=sctc.tranTable[j0][j2][1];
			j1=scte.tranTable[j0][j1][1];
			j0=stt.tranTable[j0][1];
		}while(j0!=i0 || j1!=i1 || j2!=i2 );
		return n;
	}
};


// PositionSolver holds position encoded by colourings
class PositionSolver {
	public:
	int e0,e1,e2,c0,c1,c2;
	int shp,shp2,middle;
	FullPosition fp;
	ShapeTranTable& stt;
	ShpColTranTable& scte;
	ShpColTranTable& sctc;
	PrunTable& pr1;
	PrunTable& pr2;

	int moveList[50];
	int moveLen;
	int lastTurns[6];
	bool findAll;
	bool ignoreTrans;
	// doBot() amount applied as preadf (pre-adjust D layer) before this solve
	// iteration; 0 = none.  printsol() folds this into the first (for solve) or
	// last (for generate) (mu,md) term.  The postabf is NOT pre-applied — the
	// solver finds it as a real move (enforced by the slice-point check), so the
	// whole solution including pre/post-abf comes from the search.
	int m_preadfBot{0};

	// U2/D2 ((6,0)/(0,6)) handling for the slice metric.  A U2/D2 is allowed freely
	// before the first slice (preabf) and after the last slice (postabf).  A U2/D2
	// strictly between the first and last slice ("internal") is allowed only when
	// necessary, decided per depth:
	//   * A "clean" solution (no internal U2/D2) is emitted immediately.  The first
	//     clean solution found at a depth proves internal U2/D2 is unnecessary, so
	//     m_banInternal is turned on (killing all remaining internal-U2/D2 branches)
	//     and any dirty solutions buffered so far are discarded.
	//   * A "dirty" solution (has internal U2/D2) is buffered, not emitted.  If the
	//     depth finishes with no clean solution, the buffer is emitted (U2/D2 was
	//     necessary).
	//   * At depth >= 6 slices a clean solution is always known to exist, so
	//     m_banInternal starts on and dirty branches are pruned from the outset.
	//   m_slicesDone   – slices performed so far on the current search path
	//   m_internalBad  – (6,0)/(0,6) segments strictly between first & last slice on
	//                    the current path (>0 => the current solution is "dirty")
	//   m_banInternal  – kill internal-U2/D2 branches at slice points
	//   m_cleanFound   – a clean solution was emitted at the current depth
	//   m_dirtyBuf     – dirty solutions held back at the current depth
	int m_slicesDone{0};
	int m_internalBad{0};
	bool m_banInternal{false};
	bool m_cleanFound{false};
	std::vector<std::string> m_dirtyBuf;
	bool m_cubeshape{false};

	// Emit the held-back dirty solutions for a depth that produced no clean
	// solution (internal U2/D2 was necessary).  Honors single-solution mode.
	// Returns whether at least one solution was emitted.
	bool emitDirtyBuffer() {
		bool emitted = false;
		for (const auto& s : m_dirtyBuf) {
			std::cout << s << std::flush;
			emitted = true;
			if (!findAll) break;
		}
		m_dirtyBuf.clear();
		return emitted;
	}

	PositionSolver( ShapeTranTable& stt0, ShpColTranTable& scte0, ShpColTranTable& sctc0, PrunTable& pr10, PrunTable& pr20 )
		: stt(stt0), scte(scte0), sctc(sctc0), pr1(pr10), pr2(pr20) {};
	virtual bool checkKeepCubeShape() {
		return (shp==5052 || shp==4148 || shp==5039 || shp==4163) && (shp2==5052 || shp2==4148 || shp2==5039 || shp2==4163);
	}
	void set(FullPosition& p, bool findAll0, bool ignoreTrans0){
		int cc0 = p.getCornerColouring(0);
		int cc1 = p.getCornerColouring(1);
		int cc2 = p.getCornerColouring(2);
		c0 = (cc0==-1 ? -1 : sctc.ct.choice2Idx[cc0]);
		c1 = (cc1==-1 ? -1 : sctc.ct.choice2Idx[cc1]);
		c2 = (cc2==-1 ? -1 : sctc.ct.choice2Idx[cc2]);
		int ec0 = p.getEdgeColouring(0);
		int ec1 = p.getEdgeColouring(1);
		int ec2 = p.getEdgeColouring(2);
		e0 = (ec0==-1 ? -1 : scte.ct.choice2Idx[ec0]);
		e1 = (ec1==-1 ? -1 : scte.ct.choice2Idx[ec1]);
		e2 = (ec2==-1 ? -1 : scte.ct.choice2Idx[ec2]);
		shp = stt.getShape(p.getShape(),p.getParityOdd());
		shp2 = stt.tranTable[shp][3];
		middle = p.middle;
		findAll=findAll0;
		ignoreTrans=ignoreTrans0;
		fp = p;
	};
	virtual inline int doMove(int m){
		const int mirrmv[3]={1,0,2};
		int r=0;
		if(m==0){
			r=stt.getTopTurn(shp);
		}else if(m==1){
			r=stt.getBotTurn(shp);
		}else{
			middle=-middle;
		}
		c0 = sctc.tranTable[shp][c0][m];
		c1 = sctc.tranTable[shp][c1][m];
		e0 = scte.tranTable[shp][e0][m];
		e1 = scte.tranTable[shp][e1][m];
		shp = stt.tranTable[shp][m];

		c2 = sctc.tranTable[shp2][c2][mirrmv[m]];
		e2 = scte.tranTable[shp2][e2][mirrmv[m]];
		shp2 = stt.tranTable[shp2][mirrmv[m]];
		return r;
	}
	virtual int solve(int twoGen, int extraMoves, bool keepCubeShape){
		m_cubeshape = keepCubeShape;
		// Find the valid preadf D rotations for 2-gen / pseudo-2-gen (solve guard:
		// if no compatible block exists the position can't be 2-genned).
		// For twoGen==0 this always returns {0}.
		auto preadfs = fp.findPreadf(twoGen);
		if ((twoGen == 1 || twoGen == 2) && preadfs.empty()) return 19;

		if (keepCubeShape) {
			// check that it's in cube shape and of the right parity
			if (!checkKeepCubeShape()) {
				return 19;
			}
			// keeping cube shape with a 2-gen mode also requires the corner
			// permutation to be solvable with 2-gen moves (in addition to the
			// block/preadf check above) — otherwise the search would never finish.
			// Checked once per valid preadf candidate.
			if ((twoGen == 1 || twoGen == 2) && !cornersAre2GenSolvable(fp.pos, twoGen)) {
				return 19;
			}
		}

		FullPosition fpOrig = fp;

		// Snapshot the encoded start state for each preadf candidate.  Each candidate
		// is the original position pre-adjusted by doBot(k) (D layer only — the top
		// layer is untouched, so the search still explores all U moves before the
		// first slice).  The postabf is found by the search itself, so isSolved()
		// targets canonical solved.  All candidates share the same middle (doBot
		// doesn't change it), so the depth parity is common to all.
		struct PreadfState { int e0,e1,e2,c0,c1,c2,shp,shp2,middle,preadf; };
		std::vector<PreadfState> states;
		for (int k : preadfs) {
			fp = fpOrig;
			if (k != 0) fp.doBot(k);
			set(fp, findAll, ignoreTrans);
			states.push_back({e0,e1,e2,c0,c1,c2,shp,shp2,middle,k});
		}
		const int sharedMiddle = states[0].middle;

		auto restore = [&](const PreadfState& st){
			e0=st.e0; e1=st.e1; e2=st.e2; c0=st.c0; c1=st.c1; c2=st.c2;
			shp=st.shp; shp2=st.shp2; middle=st.middle; m_preadfBot=st.preadf;
			moveLen=0; for(int i=0;i<6;i++) lastTurns[i]=0;
			m_slicesDone=0; m_internalBad=0;
		};

		unsigned long nodes=0;
		int optimalMoves = -1;
		m_dirtyBuf.clear();

		// The preadf candidates run in PARALLEL, interleaved by depth: at each depth
		// every candidate is searched before moving deeper, so the first solution
		// returned (without -a) is the globally shortest across all candidates.
		if (!specificDepths.empty()) {
			for (int depth : specificDepths) {
				if (metric == SLICE_METRIC && ((depth % 2 == 1 && sharedMiddle == 1) || (depth % 2 == 0 && sharedMiddle == -1))) {
					std::cout << "depth "<<depth<<" does not match the barflip state" << std::endl<<std::flush;
					continue;
				}
				if(verbosity>=5) std::cout<<"searching depth "<<depth<<std::endl<<std::flush;
				// Per-depth U2/D2 state: ban internal U2/D2 outright at >= 6 slices.
				m_cleanFound = false; m_dirtyBuf.clear();
				m_banInternal = (metric == SLICE_METRIC) && (depth >= 6);
				for (const auto& st : states) {
					if (stopRequested.load()) return -1;
					restore(st);
					int searchResult = search(depth, 3, &nodes, twoGen, keepCubeShape, specificAngleTop, specificAngleBot);
					if (searchResult < 0) return searchResult;
					// Slice metric stops only on a clean solution; a dirty one is buffered
					// in case no clean turns up at this depth.
					if (searchResult != 0 && !findAll && (metric != SLICE_METRIC || m_cleanFound)) { fp = fpOrig; m_preadfBot = 0; return 0; }
				}
				if (metric == SLICE_METRIC && !m_cleanFound && emitDirtyBuffer() && !findAll) { fp = fpOrig; m_preadfBot = 0; return 0; }
			}
		} else {
			// only even lengths if slice metric and middle is square
			int l=-1;
			if (metric == SLICE_METRIC && sharedMiddle==1) l=-2;
			while(true){
				l++;
				if (metric == SLICE_METRIC && sharedMiddle!=0) l++;
				if(verbosity>=5) std::cout<<"searching depth "<<l<<std::endl<<std::flush;
				// Per-depth U2/D2 state: ban internal U2/D2 outright at >= 6 slices.
				m_cleanFound = false; m_dirtyBuf.clear();
				m_banInternal = (metric == SLICE_METRIC) && (l >= 6);
				bool anySol = false;
				for (const auto& st : states) {
					if (stopRequested.load()) return -1;
					restore(st);
					int searchResult = search(l,3, &nodes, twoGen, keepCubeShape, specificAngleTop, specificAngleBot);
					if (searchResult < 0) return searchResult;
					if (searchResult != 0) {
						anySol = true;
						// Slice metric stops only on a clean solution; a dirty one is
						// buffered in case no clean turns up at this depth.
						if (!findAll && (metric != SLICE_METRIC || m_cleanFound)) { fp = fpOrig; m_preadfBot = 0; return 0; }
					}
				}
				if (metric == SLICE_METRIC && !m_cleanFound) {
					if (emitDirtyBuffer() && !findAll) { fp = fpOrig; m_preadfBot = 0; return 0; }
				}
				if (anySol && optimalMoves == -1) optimalMoves = l;
				if (optimalMoves != -1 &&
				    (l >= optimalMoves + extraMoves || (metric == SLICE_METRIC && sharedMiddle!=0 && l+1 >= optimalMoves + extraMoves)))
					break;
			}
		}

		// Restore original fp so callers see an unmodified position.
		fp = fpOrig;
		m_preadfBot = 0;
		return 0;
	}
	virtual inline bool isSolved() {
		if( shp==4163 && e0==69 && e1==44 && e2==44 && c0==69 && c1==44 && c2==44 && middle>=0 ) return true;
		else return false;
	}
	// determine if we should prune this branch of the tree
	virtual inline bool prunedOut(int l) {
		if( pr1.table[shp ][e0][c0]>l+1 ) return true;
		if( pr2.table[shp ][e1][c1]>l+1 ) return true;
		if( pr2.table[shp2][e2][c2]>l+1 ) return true;
		return false;
	}
	int search( const int l, const int lm, unsigned long *nodes, int twoGen, bool keepCubeShape, bool keepAngleTop, bool keepAngleBot){
		int i,r=0;
		if (stopRequested.load()) return -1;

		// search for l more moves. previous move was lm.
		(*nodes)++;
		if( l<0 ) return 0;

		//prune based on transformation
		// (a,b)/(c,d)/(e,f) -> (6+a,6+b)/(d,c)/(6+e,6+f)
		// qq note: this step is only done for turn metric, because the pruning steps below are ignored
		if( metric == TURN_METRIC && !ignoreTrans && twoGen == 0){
			// (a,b)/(c,d)/(e,f) -> (6+a,6+b)/(d,c)/(6+e,6+f)
			// moves changes by:
			// a,b,e,f=0/6 -> m++/m--
			i=0;
			if( lastTurns[0]==0 ) i++;
			else if( lastTurns[0]==6 ) i--;
			if( lastTurns[1]==0 ) i++;
			else if( lastTurns[1]==6 ) i--;
			if( lastTurns[4]==0 ) i++;
			else if( lastTurns[4]==6 ) i--;
			if( lastTurns[5]==0 ) i++;
			else if( lastTurns[5]==6 ) i--;
			int absTopMove = lastTurns[0]>6 ? 12-lastTurns[0] : lastTurns[0];
			int absBottomMove = lastTurns[1]>6 ? 12-lastTurns[1] : lastTurns[1];
			if( i<0 || ( i==0 && ((absTopMove + absBottomMove > 6) || (absTopMove + absBottomMove == 6 && absTopMove < absBottomMove)))) return 0;
		}

		// check if it is now solved
		if( l==0 ){
			// The final segment is the postabf (after the last slice), so any U2/D2
			// here is unrestricted — no ban.  printsol() tags the solution dirty iff
			// m_internalBad>0; the per-depth flush decides whether to keep it.
			if(isSolved()){
				printsol();
				if(verbosity>=6) std::cout<<"Nodes="<<*nodes<<std::endl<<std::flush;
				return 1;
			}else if( metric != SLICE_METRIC ) return 0;
		}

		// prune
		if(prunedOut(l)) return 0;

		// try all top layer moves
		if( lm>=2 ){
			i=doMove(0);
			do{
				// qq note: Jaap's solver pruned the transformation by only allowing U moves between
				// 0 and 5. I think it's better to do this on D, see below. We then allow any (x,0).
				int absTopMove = i>6 ? 12-i : i;
				if (absTopMove <= maxX && absTopMove <= maxTotal && (!keepAngleTop || absTopMove < 2)) {
					moveList[moveLen++]=i;
					lastTurns[4]=i;
					r+=search( metric==TURN_METRIC?l-1:metric==ANGLE_METRIC?l-absTopMove:l, 0, nodes, twoGen, keepCubeShape, keepAngleTop, keepAngleBot);
					moveLen--;
					if(r<0) return r;
					if(r!=0 && !findAll && (metric!=SLICE_METRIC || m_cleanFound)) return(r);
				}
				i+=doMove(0);
			}while( i<12);
			lastTurns[4]=0;
		}
		// try all bot layer moves
		// 2-gen / pseudo-2-gen no longer block D moves here: the solver may make any
		// D move, and the D-layer restriction is enforced at slice points instead
		// (so preadf/postabf D moves can be discovered as real moves by the search).
		if( lm!=1 ){
			i=doMove(1);
			do{
				// if we're allowed to use the transformation, and we're not doing any kind of
				// 2gen, and we're not in the last two moves, then we should skip this move if the
				// current (x,y) is worse than the alternative.
				// the logic for that is: |x| + |y| >= 7, or |x| + |y| = 6 and |y| > |x|
				int topMove = lastTurns[4];
				int absTopMove = topMove>6 ? 12-topMove : topMove;
				int absBottomMove = i>6 ? 12-i : i;
				// use the following to respect generator's solution inversion
				bool nearExemptBoundary = generator ? (m_slicesDone < 2) : (l < 2);
				if ((absBottomMove <= maxY) && (absBottomMove + absTopMove <= maxTotal) && (metric==TURN_METRIC || ignoreTrans || twoGen!=0 || nearExemptBoundary || (absTopMove + absBottomMove < 6) || (absTopMove + absBottomMove == 6 && absTopMove >= absBottomMove))  && (!keepAngleBot || absBottomMove < 2)) {
					moveList[moveLen++]=i+12;
					lastTurns[5]=i;
					r+=search( metric==TURN_METRIC?l-1:metric==ANGLE_METRIC?l-absBottomMove:l, 1, nodes, twoGen, keepCubeShape, keepAngleTop, keepAngleBot);
					moveLen--;
					if(r<0) return r;
					if(r!=0 && !findAll && (metric!=SLICE_METRIC || m_cleanFound)) return(r);
				}
				i+=doMove(1);
			}while( i<12);
			lastTurns[5]=0;
		}
		// try slice move
		if( lm!=2 && l>0){
			// The segment this slice closes is (lastTurns[4],lastTurns[5]).  A (6,0)/
			// (0,6) segment is "internal" only if a slice already preceded it
			// (m_slicesDone>=1) — the slice we're about to do supplies the following
			// slice.  Internal U2/D2 is tagged (m_internalBad) so a finished solution
			// can be classified dirty/clean; it is pruned here only once m_banInternal
			// is on (a clean solution already exists, or depth >= 6 slices).
			bool badSeg = (lastTurns[4]==6 && lastTurns[5]==0) || (lastTurns[4]==0 && lastTurns[5]==6);
			bool internalBadSeg = (metric == SLICE_METRIC) && badSeg && (m_slicesDone >= 1);
			bool block60 = internalBadSeg && m_banInternal;
			// 2-gen / pseudo-2-gen D-layer restriction.  A slice marks the end of a
			// between-slice region: the D move just performed (lastTurns[5]) must obey
			// the mode's rule (2-gen: no D; pseudo-2-gen: only D±1).  A disallowed D is
			// only legal as the postabf, i.e. AFTER the last slice — but then no slice
			// follows it, so it never reaches this check.  The preadf is already folded
			// into the start position, so the D before the first slice must be 0 too;
			// hence there is no first-slice exemption.  isSolved() is exempt so a final
			// (already-solved) state isn't spuriously killed.
			bool twoGenBlock = false;
			if (twoGen != 0) {
				int d = lastTurns[5];
				int absD = (d > 6) ? 12 - d : d;
				bool disallowedD = (twoGen == 2) ? (d != 0) : (absD > 1);
				twoGenBlock = disallowedD && !isSolved();
			}
			if (!block60 && !twoGenBlock) {
			int lt0=lastTurns[0], lt1=lastTurns[1];
			lastTurns[0]=lastTurns[2];
			lastTurns[1]=lastTurns[3];
			lastTurns[2]=lastTurns[4];
			lastTurns[3]=lastTurns[5];
			lastTurns[4]=0;
			lastTurns[5]=0;
			doMove(2);
			if (!keepCubeShape || checkKeepCubeShape()) {
				// This slice closes the current segment: a slice now precedes it, so
				// if it was a (6,0)/(0,6) and one already came before, it is internal.
				m_slicesDone++;
				if (internalBadSeg) m_internalBad++;
				moveList[moveLen++]=0;
				// note that if angle metric is defined to count slices as 0, it will sometimes miss the optimal solution because the current pruning tables count how many slices a position is away from solved
				r+=search(l-1, 2, nodes, twoGen, keepCubeShape, false, false);
				moveLen--;
				if (internalBadSeg) m_internalBad--;
				m_slicesDone--;
				if(r<0) return r;
				if(r!=0 && !findAll && (metric!=SLICE_METRIC || m_cleanFound)) return(r);
			}
			doMove(2);
			lastTurns[5]=lastTurns[3];
			lastTurns[4]=lastTurns[2];
			lastTurns[3]=lastTurns[1];
			lastTurns[2]=lastTurns[0];
			lastTurns[1]=lt1;
			lastTurns[0]=lt0;
			} // end block60 check
		}
		return r;
	}

	int normaliseMove(int m){
		while(m<0) m+=12;
		while(m>=12) m-=12;
		if( usenegative && m>6 ) m-=12;
		return m;
	}
	std::string printmove(int mu, int md){
		std::string out = "";
		if( mu!=0 || md!=0 ) {
			if( usebrackets && !karnotation ) out += "(";
			out += std::to_string(mu);
			out += ",";
			out += std::to_string(md);
			if( usebrackets && !karnotation ) out += ")";
		}
		return out;
	}
	void printsol(){
		std::string out = "";
		int tw=0, tu=0;
		int mu=0, md=0;
		int angle=0;

		if( generator ){
			for( int i=moveLen-1; i>=0; i--){
				if( moveList[i]==0 ) {
					out += printmove(mu, md);
					mu = md = 0;
					out += "/";
					tu++; tw++; angle++;
				}else if( moveList[i]<12 ){
					mu = normaliseMove(mu-moveList[i]);
					tu++;
					angle += (mu<0?-mu:mu);
				}else{
					md = normaliseMove(md+moveList[i]);
					tu++;
					angle += (md<0?-md:md);
				}
			}
			// Generator: the preadf sits at the start of the solve sequence, which
			// becomes the END of the generating sequence — its inverse (-k) is added
			// to whatever remains in md after the backward loop.
			if (m_preadfBot != 0)
				md = normaliseMove(md - m_preadfBot);
		}else{
			// Solver: preadf is the first thing that happens (a forward doBot(k) on the
			// real cube), so seed md with +k before accumulating the rest of the moves.
			md = normaliseMove(m_preadfBot);
			for( int i=0; i<moveLen; i++){
				if( moveList[i]==0 ) {
					out += printmove(mu, md);
					mu = md = 0;
					out += "/";
					tu++; tw++; angle++;
				}else if( moveList[i]<12 ){
					mu = normaliseMove(mu+moveList[i]);
					tu++;
					angle += (mu<0?-mu:mu);
				}else{
					md = normaliseMove(md-moveList[i]);
					tu++;
					angle += (md<0?-md:md);
				}
			}
			// postabf is a real move in moveList (found by the search), so nothing
			// extra to fold in here.
		}
		out += printmove(mu, md);
		// Save raw algorithm before karnotation transform (for the bridge)
		std::string rawAlg = out;
		if (karnotation)
			out = karnify(out);
		std::string line = out + "  [" + std::to_string(tw);
		if (metric != SLICE_METRIC) line += "|" + std::to_string(tu);
		if (metric == ANGLE_METRIC) line += "|" + std::to_string(angle);
		line += "]";
		if (g_extendedOutput) {
			std::string karnified = karnify(rawAlg);
			line += "  " + karnified;
			if (m_cubeshape) {
				bool initialTopA = (fp.pos[0] >= 8);
				try {
					AlgRating rating = rateAlg(rawAlg, initialTopA, 34, 100, 38, 10);
					if (rating.valid) {
						std::string safeSS = rating.sliceStart;
						if (safeSS == "\\") safeSS = "\\\\";
						else if (safeSS == "\"") safeSS = "\\\"";
						line += "  R{\"f\":" + std::to_string(rating.FINAL)
						     + ",\"ss\":\"" + safeSS + "\""
						     + ",\"p1\":" + std::to_string(rating.PHASE1)
						     + ",\"p2\":" + std::to_string(rating.PHASE2)
						     + ",\"p3\":" + std::to_string(rating.PHASE3)
						     + ",\"p4\":" + std::to_string(rating.PHASE4)
						     + ",\"eu\":" + std::to_string(rating.ergo_up)
						     + ",\"ed\":" + std::to_string(rating.ergo_down)
						     + ",\"sc\":" + std::to_string(rating.sliceCount)
						     + ",\"mv\":" + std::to_string(rating.movement)
						     + ",\"bn\":" + std::to_string(rating.bonus) + "}";
					}
				} catch (...) { }
			}
		}
		line += " \n";
		if (metric != SLICE_METRIC) { std::cout << line << std::flush; return; }
		if (m_internalBad > 0) {
			// Dirty: hold it back. In single-solution mode one buffered dirty is
			// enough (we only need a fallback if no clean solution turns up).
			if (findAll || m_dirtyBuf.empty()) m_dirtyBuf.push_back(line);
		} else {
			// Clean: emit now. The first clean of this depth makes internal U2/D2
			// provably unnecessary, so ban it from here on and drop the dirty buffer.
			if (!m_cleanFound) {
				m_cleanFound = true;
				m_banInternal = true;
				m_dirtyBuf.clear();
			}
			std::cout << line << std::flush;
		}
	}
};

// PartialositionSolver is like PositionSolver but may have some incompletely defined pieces
class PartialPositionSolver : public PositionSolver {
	int shpx, shpx2; // extra shapes to account for both possible parities

public:
	PartialPositionSolver( ShapeTranTable& stt0, ShpColTranTable& scte0, ShpColTranTable& sctc0, PrunTable& pr10, PrunTable& pr20 )
	    : PositionSolver(stt0, scte0, sctc0, pr10, pr20) {}
	bool checkKeepCubeShape() override {
		bool primary = (shp==5052 || shp==4148 || shp==5039 || shp==4163) && (shp2==5052 || shp2==4148 || shp2==5039 || shp2==4163);
		bool secondary = (shpx==5052 || shpx==4148 || shpx==5039 || shpx==4163) && (shpx2==5052 || shpx2==4148 || shpx2==5039 || shpx2==4163);
		return primary || secondary;
	}
	void set(FullPosition& p, bool findAll0, bool ignoreTrans0){
		PositionSolver::set(p, findAll0, ignoreTrans0);
		shpx = stt.getShape(p.getShape(),!p.getParityOdd());
		shpx2 = stt.tranTable[shpx][3];
	};
	inline int doMove(int m) override {
		const int mirrmv[3]={1,0,2};
		int r=0;
		if(m==0){
			r=stt.getTopTurn(shp);
			fp.doTop(r);
		}else if(m==1){
			r=stt.getBotTurn(shp);
			fp.doBot(-r);
		}else{
			middle=-middle;
			fp.doSlice();
		}
		// only update c0/c1/e0/e1/c2/e2 if they are not -1
		if (c0>-1) c0 = sctc.tranTable[shp][c0][m];
		if (c1>-1) c1 = sctc.tranTable[shp][c1][m];
		if (e0>-1) e0 = scte.tranTable[shp][e0][m];
		if (e1>-1) e1 = scte.tranTable[shp][e1][m];
		shp = stt.tranTable[shp][m];
		shpx = stt.tranTable[shpx][m];

		if (c2>-1) c2 = sctc.tranTable[shp2][c2][mirrmv[m]];
		if (e2>-1) e2 = scte.tranTable[shp2][e2][mirrmv[m]];
		shp2 = stt.tranTable[shp2][mirrmv[m]];
		shpx2 = stt.tranTable[shpx2][mirrmv[m]];
		return r;
	}
	int solve(int twoGen, int extraMoves, bool keepCubeShape) override {
		m_cubeshape = keepCubeShape;
		// Partial-aware preadf detection doubles as the 2-gen / p2g solve guard:
		// twoGenPreadf understands U/V/W/X/Y/Z pieces, so it returns every rotation
		// that can bring a (possibly partially-specified) solved block to the frozen
		// bottom-left.  Empty => not 2-genable.  twoGen==0 returns {0}.
		auto preadfs = fp.findPreadf(twoGen);
		if ((twoGen == 1 || twoGen == 2) && preadfs.empty()) return 19;

		if (keepCubeShape) {
			// check that it's in cube shape and of the right parity, and that the
			// corner permutation is 2-gen-solvable (partial-aware, once per preadf).
			if (!checkKeepCubeShape()) {
				return 19;
			}
			if ((twoGen == 1 || twoGen == 2) && !cornersAre2GenSolvable(fp.pos, twoGen)) {
				return 19;
			}
		}

		FullPosition fpOrig = fp;

		// Snapshot the full per-preadf start state.  PartialPositionSolver::doMove
		// mutates fp and the extra shapes during search, and isSolved() reads fp, so
		// everything must be restored before each search.  All candidates share the
		// same middle (doBot doesn't change it), so the depth parity is common.
		struct PreadfState {
			FullPosition fp;
			int e0,e1,e2,c0,c1,c2,shp,shp2,shpx,shpx2,middle,preadf;
		};
		std::vector<PreadfState> states;
		for (int k : preadfs) {
			fp = fpOrig;
			if (k != 0) fp.doBot(k);
			set(fp, findAll, ignoreTrans);
			states.push_back({fp, e0,e1,e2,c0,c1,c2,shp,shp2,shpx,shpx2,middle,k});
		}
		const int sharedMiddle = states[0].middle;

		auto restore = [&](const PreadfState& st){
			fp=st.fp;
			e0=st.e0; e1=st.e1; e2=st.e2; c0=st.c0; c1=st.c1; c2=st.c2;
			shp=st.shp; shp2=st.shp2; shpx=st.shpx; shpx2=st.shpx2;
			middle=st.middle; m_preadfBot=st.preadf;
			moveLen=0; for(int i=0;i<6;i++) lastTurns[i]=0;
			m_slicesDone=0; m_internalBad=0;
		};

		unsigned long nodes=0;
		int optimalMoves = -1;
		m_dirtyBuf.clear();

		// preadf candidates run in parallel, interleaved by depth (see PositionSolver::solve).
		if (!specificDepths.empty()) {
			for (int depth : specificDepths) {
				if (metric == SLICE_METRIC && ((depth % 2 == 1 && sharedMiddle == 1) || (depth % 2 == 0 && sharedMiddle == -1))) {
					std::cout << "depth "<<depth<<" does not match the barflip state" << std::endl<<std::flush;
					continue;
				}
				if(verbosity>=5) std::cout<<"searching depth "<<depth<<std::endl<<std::flush;
				// Per-depth U2/D2 state: ban internal U2/D2 outright at >= 6 slices.
				m_cleanFound = false; m_dirtyBuf.clear();
				m_banInternal = (metric == SLICE_METRIC) && (depth >= 6);
				for (const auto& st : states) {
					if (stopRequested.load()) return -1;
					restore(st);
					int searchResult = search(depth, 3, &nodes, twoGen, keepCubeShape, specificAngleTop, specificAngleBot);
					if (searchResult < 0) return searchResult;
					// Slice metric stops only on a clean solution; a dirty one is buffered
					// in case no clean turns up at this depth.
					if (searchResult != 0 && !findAll && (metric != SLICE_METRIC || m_cleanFound)) { fp = fpOrig; m_preadfBot = 0; return 0; }
				}
				if (metric == SLICE_METRIC && !m_cleanFound && emitDirtyBuffer() && !findAll) { fp = fpOrig; m_preadfBot = 0; return 0; }
			}
		} else {
			int l=-1;
			if( metric==SLICE_METRIC && sharedMiddle==1 ) l=-2;
			while(true){
				l++;
				if( metric==SLICE_METRIC && sharedMiddle!=0 ) l++;
				if(verbosity>=5) std::cout<<"searching depth "<<l<<std::endl<<std::flush;
				// Per-depth U2/D2 state: ban internal U2/D2 outright at >= 6 slices.
				m_cleanFound = false; m_dirtyBuf.clear();
				m_banInternal = (metric == SLICE_METRIC) && (l >= 6);
				bool anySol = false;
				for (const auto& st : states) {
					if (stopRequested.load()) return -1;
					restore(st);
					int searchResult = search(l,3, &nodes, twoGen, keepCubeShape, specificAngleTop, specificAngleBot);
					if (searchResult < 0) return searchResult;
					if (searchResult != 0) {
						anySol = true;
						// Slice metric stops only on a clean solution; a dirty one is
						// buffered in case no clean turns up at this depth.
						if (!findAll && (metric != SLICE_METRIC || m_cleanFound)) { fp = fpOrig; m_preadfBot = 0; return 0; }
					}
				}
				if (metric == SLICE_METRIC && !m_cleanFound) {
					if (emitDirtyBuffer() && !findAll) { fp = fpOrig; m_preadfBot = 0; return 0; }
				}
				if (anySol && optimalMoves == -1) optimalMoves = l;
				if (optimalMoves != -1 &&
				    (l >= optimalMoves + extraMoves || (metric==SLICE_METRIC && sharedMiddle!=0 && l+1 >= optimalMoves + extraMoves)))
					break;
			}
		}

		fp = fpOrig;
		m_preadfBot = 0;
		return 0;
	}
	inline bool isSolved() override {
		return (fp.matchesSolved() && middle>=0);
	}
	// determine if we should prune this branch of the tree
	// we should have a shape-only pruning table
	inline bool prunedOut(int l) override {
		if (e0>-1 && c0>-1) {
			if( pr1.table[shp ][e0][c0]>l+1 && pr1.table[shpx][e0][c0]>l+1) return true;
		}
		if (e1>-1 && c1>-1) {
			if( pr2.table[shp ][e1][c1]>l+1 && pr2.table[shpx][e1][c1]>l+1) return true;
		}
		if (e2>-1 && c2>-1) {
			if( pr2.table[shp2][e2][c2]>l+1 && pr2.table[shpx2][e2][c2]>l+1) return true;
		}
		return false;
	}
};

int show(int e){
	std::cerr<<errors[e-1]<<std::endl;
	return(e);
}

// Standalone cubeshape check: edges at positions i%3==r for some r, both layers.
// Does not need ShapeTranTable — works directly on the raw position array.
static bool isInCubeshapeRaw(const int pos[24]) {
	for (int base = 0; base < 24; base += 12) {
		bool layerOk = false;
		for (int r = 0; r < 3; r++) {
			bool match = true;
			for (int i = 0; i < 12; i++) {
				if ((i % 3 == r) != (pos[base + i] >= 8)) { match = false; break; }
			}
			if (match) { layerOk = true; break; }
		}
		if (!layerOk) return false;
	}
	return true;
}

// Pre-validate position against keepCubeShape + twoGen constraints.
// Returns 0 if OK, error code (19) if unsolvable with these constraints.
// Safe to call before pruning tables exist.
static int preValidate(FullPosition& p, bool keepCubeShape, int twoGen) {
	if (!keepCubeShape) return 0;
	if (!isInCubeshapeRaw(p.pos)) return 19;
	if (!p.isPartial() && p.getParityOdd()) return 19;
	if ((twoGen == 1 || twoGen == 2) && !cornersAre2GenSolvable(p.pos, twoGen)) return 19;
	return 0;
}

int parseInteger(const char* s){
	int n=0;
	while( *s!='\0' ){
		if( *s<'0' || *s>'9' ) return -1;
		n = n*10 + (*s -'0');
		s++;
	}
	return n;
}

void help(){
	std::cout<<"Square-1 Optimizer v2 Usage:"<<std::endl;
	std::cout<<"  sq1opt <switches> <position>"<<std::endl;
	std::cout<<"  sq1opt <switches> <movesequence>"<<std::endl;
	std::cout<<"  sq1opt <switches>"<<std::endl;
	std::cout<<std::endl;
	std::cout<<"<position> is a string encoding a particular position. For example"<<std::endl;
	std::cout<<"   A1B2C3D45E6F7G8H- is the solved position. Letters represent corners, numbers"<<std::endl;
	std::cout<<"   the edges, starting from the front seam clockwise around the top layer and"<<std::endl;
	std::cout<<"   then clockwise around the bottom layer. Optionally, the middle layer is"<<std::endl;
	std::cout<<"   denoted by a - for a square and / for kite shape."<<std::endl;
	std::cout<<"   You can also partially define pieces:"<<std::endl;
	std::cout<<"   U is a top corner, V is a bottom corner, W is any corner,"<<std::endl;
	std::cout<<"   X is a top edge,   Y is a bottom edge,   Z is any edge."<<std::endl;
	std::cout<<"<movesequence> is a string encoding a sequence of moves. Layer turns are"<<std::endl;
	std::cout<<"   denoted by (x,y) where x and y are integers indicating that the top and"<<std::endl;
	std::cout<<"   bottom layers are turned by x and y twelths of a full circle. Positive"<<std::endl;
	std::cout<<"   numbers are clockwise turns, negative anti-clockwise."<<std::endl;
	std::cout<<"<switches> are one of more of the following command line switches:"<<std::endl;
	std::cout<<"   -es    Use slice metric (only slices count as moves)."<<std::endl;
	std::cout<<"   -em    Use move/turn metric (layer turns count; this is the default)."<<std::endl;
	std::cout<<"   -ea    Use angle metric."<<std::endl;
	std::cout<<"   -a<n>  Generate all optimal sequences, not just the first one found."<<std::endl;
	std::cout<<"          If n is given, also find solutions with up to n extra moves."<<std::endl;
	std::cout<<"   -x     Ignore the equivalence a,b/c,d/e,f = 6+a,6+b/d,c/6+e,6+f"<<std::endl;
	std::cout<<"   -m     Ignore the middle layer shape."<<std::endl;
	std::cout<<"   -b     Use brackets in output around layer turns"<<std::endl;
	std::cout<<"   -r<n>  Solve n random positions, or infinitely many if n is 0 or missing."<<std::endl;
	std::cout<<"   -v<n>  Set verbosity, between 0 (minimal output) to 7 (full output)"<<std::endl;
	std::cout<<"   -h     Show this help"<<std::endl;
	std::cout<<"   -g     Input/Output generating move sequences rather than solutions."<<std::endl;
	std::cout<<"   -i<fn> Use as input each line from the file with filename <fn>."<<std::endl;
	std::cout<<"   -2     2gen - no bottom layer moves."<<std::endl;
	std::cout<<"   -p     Pseudo 2gen - only allow bottom layer moves of 1, 0, -1."<<std::endl;
	std::cout<<"   -c     Only generate algs that stay in a square/square cubeshape."<<std::endl;
	std::cout<<"   -k     Output algs in Karnotation. Ignores ABF."<<std::endl;
	std::cout<<"   -ob    Normalize AUF on both pre-ABF and post-ABF moves."<<std::endl;
	std::cout<<"   -oe    Normalize AUF on the pre-ABF move only (before first slice)."<<std::endl;
	std::cout<<"   -os    Normalize AUF on the post-ABF move only (after last slice)."<<std::endl;
	std::cout<<"   -nb    Lock both layer angles on pre-ABF (top and bottom)."<<std::endl;
	std::cout<<"   -nu    Lock top layer angle on pre-ABF only."<<std::endl;
	std::cout<<"   -nd    Lock bottom layer angle on pre-ABF only."<<std::endl;
	std::cout<<"   -nn    No layer angle lock (default)."<<std::endl;
	std::cout<<"   -X>n>  Only allow top layer turns of a maximum of n in either direction."<<std::endl;
	std::cout<<"   -Y>n>  Only allow bottom layer turns of a maximum of n in either direction."<<std::endl;
	std::cout<<"   -Z>n>  Only allow turns of a maximum of n total turn amount (abs(X) + abs(Y))."<<std::endl;
}


// -w|u=slice/turn metric  -a=all  -m=ignore middle
int sq1optMain(int argc, char* argv[]){
	resetSolverOptions();
	bool ignoreMid=false;
	bool ignoreTrans=false;
	bool findAll=false;
	int twoGen = 0; // 0 = false, 1 = pseudo 2gen, 2 = true 2gen
	int numpos = -1;
	char *inpFile=NULL;
	int posArg=-1;
	usenegative=true; // why would you not want negative turns?
	int extraMoves = 0;
	bool keepCubeShape = false;
	int parsedValue = 0;
	for( int i=1; i<argc; i++){
		if( argv[i][0]=='-' ){
			switch( argv[i][1] ){
				case 'e':
				case 'E':
					if (argv[i][2]=='s'||argv[i][2]=='S') metric=SLICE_METRIC;
					else if (argv[i][2]=='m'||argv[i][2]=='M') metric=TURN_METRIC;
					else if (argv[i][2]=='a'||argv[i][2]=='A') metric=ANGLE_METRIC;
					else return show(1);
					break;
				case 'x':
					ignoreTrans=true; break;
				case 'a':
				case 'A':
					findAll=true;
					extraMoves = parseInteger( argv[i]+2 );
					if( extraMoves<0 ) return show(15);
					break;
				case 'm':
				case 'M':
					ignoreMid=true; break;
				case 'b':
				case 'B':
					usebrackets=true; break;
				case 'r':
				case 'R':
					numpos = parseInteger( argv[i]+2 );
					if( numpos<0 ) return show(15);
					break;
				case 'v':
				case 'V':
					verbosity = parseInteger( argv[i]+2 );
					if( verbosity<0 ) return show(15);
					break;
				case 'h':
				case 'H':
					help();
					return 0;
				case 'g':
				case 'G':
					generator = true;
					break;
				case 'i':
				case 'I':
					inpFile=argv[i]+2; break;
				case 'p':
				case 'P':
					twoGen = 1;
					break;
				case '2':
					twoGen = 2;
					break;
				case 'c':
				case 'C':
					keepCubeShape = true;
					break;
				case 'k':
				case 'K':
					karnotation = true;
					break;
				case 'n':
				case 'N':
					if (argv[i][2]=='b'||argv[i][2]=='B') { specificAngleTop=true; specificAngleBot=true; }
					else if (argv[i][2]=='u'||argv[i][2]=='U') specificAngleTop=true;
					else if (argv[i][2]=='d'||argv[i][2]=='D') specificAngleBot=true;
					else if (argv[i][2]=='n'||argv[i][2]=='N') { specificAngleTop=false; specificAngleBot=false; }
					else return show(1);
					break;
				case 'X':
					parsedValue = parseInteger(argv[i]+2);
					if (parsedValue >= 0 && parsedValue <= 6) maxX = parsedValue;
					break;
				case 'Y':
					parsedValue = parseInteger(argv[i]+2);
					if (parsedValue >= 0 && parsedValue <= 6) maxY = parsedValue;
					break;
				case 'Z':
					parsedValue = parseInteger(argv[i]+2);
					if (parsedValue >= 1 && parsedValue <= 12) maxTotal = parsedValue;
					break;
				case 'd':
				case 'D':
					{
						std::string ds(argv[i]+2);
						std::stringstream ss(ds);
						std::string token;
						while(std::getline(ss, token, ',')) {
							token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
							if (!token.empty()) {
								try { specificDepths.push_back(std::stoi(token)); } catch(...) {}
							}
						}
					}
					break;
				default:
					return show(1);
			}
		}else if( posArg<0 ){
			posArg = i;
		}else{
			return show(2);
		}
	}

	// don't use the equivalence if we want to limit move amounts
	if (maxX != 6 || maxY != 6 || maxTotal != 12) ignoreTrans = true;

	FullPosition p;
	std::ifstream is;
	bool havePosition = false;
	// Use directly injected position if available (bypasses string encoding/decoding)
	if( s_hasInjectedPosition ){
		p.set(s_injectedPos, s_injectedMiddle);
		s_hasInjectedPosition = false;
		havePosition = true;
	}else if( posArg>=0 ){
		int r=p.parseInput(argv[posArg]);
		if(r) return show(r);
		havePosition = true;
	}else if( inpFile!=NULL ){
		is.open(inpFile);
		if(is.fail()) return show(3);
	}else if( numpos<0 ){
		help();
		return 0;
	}

	// now we have a position p to solve (if posArg>=0 or injected)

	// Pre-validate: catch impossible constraints before expensive table init
	if (havePosition) {
		int pre = preValidate(p, keepCubeShape, twoGen);
		if (pre) return show(pre);
	}

	if(verbosity>=3) std::cout << "Initializing..."<<std::endl;
	// calculate transition tables
	ChoiceTable ct;
	if(verbosity>=4) std::cout << "  5. Shape transition table"<<std::endl;
	ShapeTranTable st;
	if(verbosity>=4) std::cout << "  4. Coloring transition table #1"<<std::endl;
	ShpColTranTable scte( st, ct, true );
	if(verbosity>=4) std::cout << "  3. Coloring transition table #2"<<std::endl;
	ShpColTranTable sctc( st, ct, false );

	//calculate pruning tables for two colourings
	FullPosition q;
	if(verbosity>=4) std::cout << "  2. Coloring pruning table #1"<<std::endl;
	PrunTable pr1(q, 0, st,scte,sctc );
	if(verbosity>=4) std::cout << "  1. Coloring pruning table #2"<<std::endl;
	PrunTable pr2(q, 1, st,scte,sctc );
	if(verbosity>=4) std::cout << "  0. Finished."<<std::endl;
	PositionSolver ps( st, scte, sctc, pr1, pr2 );
	PartialPositionSolver pps( st, scte, sctc, pr1, pr2 );

	if(verbosity>=2){
		std::cout<<"Flags: "<<(metric==TURN_METRIC?"Move":metric==SLICE_METRIC?"Slice":"Angle")<<" Metric, ";
		std::cout<<"Find "<< (findAll? "every ":"first ");
		std::cout<< (generator? "generator":"solution");
		if (twoGen == 1) {
			std::cout << ", Pseudo 2gen";
		} else if (twoGen == 2) {
			std::cout << ", 2gen";
		}
		if (keepCubeShape) {
			std::cout << ", Keep Cube Shape";
		}
		std::cout<< std::endl;
	}

	srand( (unsigned)time( NULL ) );
	char buffer[2000];
	do{
		clock_t now = clock();
		if( posArg<0 ){
			if( inpFile!=NULL ){
				is.getline(buffer,1999);
				int r=p.parseInput(buffer);
				if(r) {
					show(r);
					continue;
				}
			}else{
				p.random(twoGen, keepCubeShape);
			}
		}
		if( ignoreMid ) p.middle=0;

		//show position
		if(verbosity>=1){
			std::cout<<"Position: ";
			p.print();
			std::cout<<std::endl;
		}

		// Pre-validate per-position (file/random input) — skip expensive solve if impossible
		if (posArg < 0) {
			int pre = preValidate(p, keepCubeShape, twoGen);
			if (pre) { show(pre); continue; }
		}

		if (p.isPartial()) {
			// convert position to colour encoding
			pps.set(p, findAll, ignoreTrans);

			//solve position
			int r = pps.solve(twoGen, extraMoves, keepCubeShape);
			if (r < 0) return 130;
			if (r) show(r);
		} else {
			// convert position to colour encoding
			ps.set(p, findAll, ignoreTrans);

			//solve position
			int r = ps.solve(twoGen, extraMoves, keepCubeShape);
			if (r < 0) return 130;
			if (r) show(r);
		}

		if (verbosity>=6) std::cout << "Time: " << (clock() - now);
		std::cout<<std::endl;
	}while( posArg<0 && ( (inpFile!=NULL && !is.eof() ) || (inpFile==NULL && (numpos==0 || numpos-- > 1)) ));

	return(0);
}

#ifndef SQ1OPT_LIBRARY
int main(int argc, char* argv[])
{
	return sq1optMain(argc, argv);
}
#endif





/*

ttshp: 7356*3 ints.   done

tt: 70*7356*3 chars for edges
tt: 70*7356*3 chars for corners

pt: 70*70*7356 chars colour 1,2
pt: 70*70*7356 chars colour 3

*/
