#include "float.h"

#ifdef WINDOWS
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "MyAI.h"

#define TIME_LIMIT 9.5

#define WIN 1.0
#define DRAW 0.2
#define LOSE 0.0
#define BONUS 0.3
#define BONUS_MAX_PIECE 8

#define OFFSET (WIN + BONUS)

#define NOEATFLIP_LIMIT 60
#define POSITION_REPETITION_LIMIT 3

MyAI::MyAI(void){}

MyAI::~MyAI(void){}

bool MyAI::protocol_version(const char* data[], char* response){
	strcpy(response, "1.0.0");
  return 0;
}

bool MyAI::name(const char* data[], char* response){
	strcpy(response, "MyAI");
	return 0;
}

bool MyAI::version(const char* data[], char* response){
	strcpy(response, "1.0.0");
	return 0;
}

bool MyAI::known_command(const char* data[], char* response){
  for(int i = 0; i < COMMAND_NUM; i++){
		if(!strcmp(data[0], commands_name[i])){
			strcpy(response, "true");
			return 0;
		}
	}
	strcpy(response, "false");
	return 0;
}

bool MyAI::list_commands(const char* data[], char* response){
  for(int i = 0; i < COMMAND_NUM; i++){
		strcat(response, commands_name[i]);
		if(i < COMMAND_NUM - 1){
			strcat(response, "\n");
		}
	}
	return 0;
}

bool MyAI::quit(const char* data[], char* response){
  fprintf(stderr, "Bye\n"); 
	return 0;
}

bool MyAI::boardsize(const char* data[], char* response){
  fprintf(stderr, "BoardSize: %s x %s\n", data[0], data[1]); 
	return 0;
}

bool MyAI::reset_board(const char* data[], char* response){
	this->Red_Time = -1; // unknown
	this->Black_Time = -1; // unknown
	this->initBoardState();
	return 0;
}

bool MyAI::num_repetition(const char* data[], char* response){
  return 0;
}

bool MyAI::num_moves_to_draw(const char* data[], char* response){
  return 0;
}

bool MyAI::move(const char* data[], char* response){
  char move[6];
	sprintf(move, "%s-%s", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::flip(const char* data[], char* response){
  char move[6];
	sprintf(move, "%s(%s)", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::genmove(const char* data[], char* response){
	// set color
	if(!strcmp(data[0], "red")){
		this->Color = RED;
	}else if(!strcmp(data[0], "black")){
		this->Color = BLACK;
	}else{
		this->Color = 2;
	}
	// genmove
  char move[6];
	this->generateMove(move);
	sprintf(response, "%c%c %c%c", move[0], move[1], move[3], move[4]);
	return 0;
}

bool MyAI::game_over(const char* data[], char* response){
  fprintf(stderr, "Game Result: %s\n", data[0]); 
	return 0;
}

bool MyAI::ready(const char* data[], char* response){
  return 0;
}

bool MyAI::time_settings(const char* data[], char* response){
  return 0;
}

bool MyAI::time_left(const char* data[], char* response){
  if(!strcmp(data[0], "red")){
		sscanf(data[1], "%d", &(this->Red_Time));
	}else{
		sscanf(data[1], "%d", &(this->Black_Time));
	}
	fprintf(stderr, "Time Left(%s): %s\n", data[0], data[1]); 
	return 0;
}

bool MyAI::showboard(const char* data[], char* response){
  Pirnf_Chessboard();
	return 0;
}


// *********************** AI FUNCTION *********************** //

int MyAI::GetFin(char c)
{
	static const char skind[]={'-','K','G','M','R','N','C','P','X','k','g','m','r','n','c','p'};
	for(int f=0;f<16;f++)if(c==skind[f])return f;
	return -1;
}

int MyAI::ConvertChessNo(int input)
{
	switch(input)
	{
	case 0:
		return CHESS_EMPTY;
		break;
	case 8:
		return CHESS_COVER;
		break;
	case 1:
		return 6;
		break;
	case 2:
		return 5;
		break;
	case 3:
		return 4;
		break;
	case 4:
		return 3;
		break;
	case 5:
		return 2;
		break;
	case 6:
		return 1;
		break;
	case 7:
		return 0;
		break;
	case 9:
		return 13;
		break;
	case 10:
		return 12;
		break;
	case 11:
		return 11;
		break;
	case 12:
		return 10;
		break;
	case 13:
		return 9;
		break;
	case 14:
		return 8;
		break;
	case 15:
		return 7;
		break;
	}
	return -1;
}


void MyAI::initBoardState()
{	
	int iPieceCount[14] = {5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1};
	memcpy(main_chessboard.CoverChess,iPieceCount,sizeof(int)*14);
	main_chessboard.Red_Chess_Num = 16;
	main_chessboard.Black_Chess_Num = 16;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	//convert to my format
	int Index = 0;
	for(int i=0;i<8;i++)
	{
		for(int j=0;j<4;j++)
		{
			main_chessboard.Board[Index] = CHESS_COVER;
			Index++;
		}
	}
	Pirnf_Chessboard();
}

void MyAI::generateMove(char move[6])
{
	/* generateMove Call by reference: change src,dst */
	int StartPoint = 0;
	int EndPoint = 0;

	double t = -DBL_MAX;
	
#ifdef WINDOWS
	begin = clock();
#else
	gettimeofday(&begin, 0);
#endif

	// iterative-deeping, start from 3, time limit = <TIME_LIMIT> sec
	// why depth start at 3 ??
	for(int depth = 3; !isTimeUp(); ++depth){
		this->node = 0;
		int best_move_tmp; double t_tmp;

		// run Nega-max
		t_tmp = Nega_max(this->main_chessboard, &best_move_tmp, this->Color, 0, depth, -DBL_MAX, DBL_MAX);
		t_tmp -= OFFSET; // rescale

		// check score
		// if search all nodes
		// replace the move and score
		if(!this->timeIsUp){
			t = t_tmp;
			StartPoint = best_move_tmp/100;
			EndPoint   = best_move_tmp%100;
			sprintf(move, "%c%c-%c%c",'a'+(StartPoint%4),'1'+(7-StartPoint/4),'a'+(EndPoint%4),'1'+(7-EndPoint/4));
			// 'a' + 2 = c
		}
		// U: Undone
		// D: Done
		fprintf(stderr, "[%c] Depth: %2d, Node: %10d, Score: %+1.5lf, Move: %s\n", (this->timeIsUp ? 'U' : 'D'), 
			depth, node, t, move);
		fflush(stderr);

		// game finish !!!
		if(t >= WIN){
			break;
		}
	}
	
	char chess_Start[4]="";
	char chess_End[4]="";
	Pirnf_Chess(main_chessboard.Board[StartPoint],chess_Start);
	Pirnf_Chess(main_chessboard.Board[EndPoint],chess_End);
	printf("My result: \n--------------------------\n");
	printf("Nega max: %lf (node: %d)\n", t, this->node);
	printf("(%d) -> (%d)\n",StartPoint,EndPoint);
	printf("<%s> -> <%s>\n",chess_Start,chess_End); // if chess_End != null, then attack happened
	printf("move:%s\n",move);
	printf("--------------------------\n");
	this->Pirnf_Chessboard();
}

int ConvertChessToPiecePosIndex(int chess_num) {
	int tmp;
	if (chess_num % 7 == 6) { // king
		tmp = (chess_num + chess_num / 7) * 2 + 3;
	} else {
		tmp = (chess_num + chess_num / 7) * 2 + 4;
	}
	return tmp;
}

void MyAI::MakeMove(ChessBoard* chessboard, const int move, const int chess){
	int src = move/100, dst = move%100;
	if(src == dst){ // flip
		chessboard->Board[src] = chess;
		chessboard->CoverChess[chess]--;
		chessboard->NoEatFlip = 0;
		chessboard->piece[chess] |= (1 << src);
		chessboard->color[chess / 7] |= (1 << src);
	}else { // move
		if(chessboard->Board[dst] != CHESS_EMPTY){ // attack
			if(chessboard->Board[dst] / 7 == 0){ // dst is red
				(chessboard->Red_Chess_Num)--;
			}else{ // dst is black
				(chessboard->Black_Chess_Num)--;
			}
			chessboard->color[chessboard->Board[dst] / 7] ^= (1 << dst);
			chessboard->piece[chessboard->Board[dst]] ^= (1 << dst);
			chessboard->NoEatFlip = 0;
		}else{ // not attack
			chessboard->NoEatFlip += 1;
		}
		chessboard->color[chessboard->Board[src] / 7] = 
		(chessboard->color[chessboard->Board[src] / 7] ^ (1 << src)) | (1 << dst);

		chessboard->piece[chessboard->Board[src]] = 
		(chessboard->piece[chessboard->Board[src]] ^ (1 << src)) | (1 << dst);
		
		chessboard->Board[dst] = chessboard->Board[src];
		chessboard->Board[src] = CHESS_EMPTY;
	}
	chessboard->History[chessboard->HistoryCount++] = move;
}

void MyAI::MakeMove(ChessBoard* chessboard, const char move[6])
{ 
	int src, dst, m;
	src = ('8'-move[1])*4+(move[0]-'a');
	if(move[2]=='('){ // flip
		m = src*100 + src;
		printf("# call flip(): flip(%d,%d) = %d\n",src, src, GetFin(move[3])); 
	}else { // move
		dst = ('8'-move[4])*4+(move[3]-'a');
		m = src*100 + dst;
		printf("# call move(): move : %d-%d \n", src, dst);
	}
	MakeMove(chessboard, m, ConvertChessNo(GetFin(move[3])));
	Pirnf_Chessboard();
}

int index32[32] = {31, 0, 1, 5, 2, 16, 27, 6, 3, 14, 17, 19, 28, 11, 7, 21, 30, 4, 15, 26, 13, 18, 10, 20, 29, 25, 12, 9, 24, 8, 23, 22};

int GetIndex(unsigned int mask){
	return index32[(mask * 0x08ED2BE6) >> 27];
}

// int MyAI::Expand(const int* board, const unsigned int* piece, const int color,int *Result)
int MyAI::Expand(const int* board, const unsigned int* piece, const unsigned int my_color, const int color, int *Result) {
	int ResultCount = 0;
	unsigned int pMoves[32] = {
		0x00000012, 0x00000025, 0x0000004A, 0x00000084,
		0x00000121, 0x00000252, 0x000004A4, 0x00000848,
		0x00001210, 0x00002520, 0x00004A40, 0x00008480,
		0x00012100, 0x00025200, 0x0004A400, 0x00084800,
		0x00121000, 0x00252000, 0x004A4000, 0x00848000,
		0x01210000, 0x02520000, 0x04A40000, 0x08480000,
		0x12100000, 0x25200000, 0x4A400000, 0x84800000,
		0x21000000, 0x52000000, 0xA4000000, 0x48000000
	};
	
	for (int i = 6; i >= 0; i--) {
		unsigned int p = piece[color * 7 + i];
		// printf("------------------\n");
		while (p) {
			unsigned int dst = 0;
			// if there are many same piece, get the last bit first
			unsigned int mask = p & (-p); // Least Significant 1 Bit (LS1B)
			p ^= mask;
			
			unsigned int src = GetIndex(mask);
			if (i == 6) { // king
				// dst is not my piece and not pawn
				dst = pMoves[src] & (~ my_color) & (~ piece[(color^1) * 7]);
			} else if (i == 5) { // guard
				// dst is not my piece and not king
				dst = pMoves[src] & (~ my_color) & (~ piece[(color^1) * 7 + 6]);
			} else if (i == 4) { // minister
				// dst is not my piece and not king and not guard
				dst = pMoves[src] & (~ my_color) & (~ piece[(color^1) * 7 + 6]) & (~ piece[(color^1) * 7 + 5]);
			} else if (i == 3) { // rook
				dst = pMoves[src] & (~ my_color) & (~ piece[(color^1) * 7 + 6]) & (~ piece[(color^1) * 7 + 5])
												 & (~ piece[(color^1) * 7 + 4]);
			} else if (i == 2) { // knight
				dst = pMoves[src] & (~ my_color) & (~ piece[(color^1) * 7 + 6]) & (~ piece[(color^1) * 7 + 5])
												 & (~ piece[(color^1) * 7 + 4]) & (~ piece[(color^1) * 7 + 3]);
			// } else if (i == 1) { // cannon
			// 	// not this time
			} else if (i == 0) { // pawn
				dst = pMoves[src] & (~ my_color) & (~ piece[(color^1) * 7 + 6]) & (~ piece[(color^1) * 7 + 5])
												 & (~ piece[(color^1) * 7 + 4]) & (~ piece[(color^1) * 7 + 3])
												 & (~ piece[(color^1) * 7 + 2]) & (~ piece[(color^1) * 7 + 1]);
			}
			while (dst) {
				unsigned int mask2 = dst & (-dst); // Least Significant 1 Bit (LS1B)
				dst ^= mask2;
				Result[ResultCount] = src*100 + GetIndex(mask2);
				ResultCount++;
			}
		}
	}
	// printf("Count: %d\n", ResultCount);
	return ResultCount;
}

// Referee (check if it is a legal move)
bool MyAI::Referee(const int* chess, const int from_location_no, const int to_location_no, const int UserId)
{
	// int MessageNo = 0;
	bool IsCurrent = true;
	int from_chess_no = chess[from_location_no];
	int to_chess_no = chess[to_location_no];
	int from_row = from_location_no / 4;
	int to_row = to_location_no / 4;
	int from_col = from_location_no % 4;
	int to_col = to_location_no % 4;

	if(from_chess_no < 0 || ( to_chess_no < 0 && to_chess_no != CHESS_EMPTY) )
	{  
		// MessageNo = 1;
		//strcat(Message,"**no chess can move**");
		//strcat(Message,"**can't move darkchess**");
		IsCurrent = false;
	}
	else if (from_chess_no >= 0 && from_chess_no / 7 != UserId)
	{
		// MessageNo = 2;
		//strcat(Message,"**not my chess**");
		IsCurrent = false;
	}
	else if((from_chess_no / 7 == to_chess_no / 7) && to_chess_no >= 0)
	{
		// MessageNo = 3;
		//strcat(Message,"**can't eat my self**");
		IsCurrent = false;
	}
	//check attack
	else if(to_chess_no == CHESS_EMPTY && abs(from_row-to_row) + abs(from_col-to_col)  == 1)//legal move
	// if there is not any piece at destination (legal move)	
	{
		IsCurrent = true;
	}	
	else if(from_chess_no % 7 == 1 ) //judge gun (the piece I want to move is gun)
	{
		int row_gap = from_row-to_row;
		int col_gap = from_col-to_col;
		int between_Count = 0;
		//slant
		if(from_row-to_row == 0 || from_col-to_col == 0)
		{
			// it is not reasonable to determine the destination on row first

			//row
			// if starting point and destination are on the same row, 
			// then count the piece amount in between
			if(row_gap == 0) 
			{
				for(int i=1;i<abs(col_gap);i++)
				{
					int between_chess;
					if(col_gap>0)
						between_chess = chess[from_location_no-i] ;
					else
						between_chess = chess[from_location_no+i] ;
					if(between_chess != CHESS_EMPTY)
						between_Count++;
				}
			}
			//column
			// if starting point and destination are on the same column, 
			// then count the piece amount in between
			else
			{
				for(int i=1;i<abs(row_gap);i++)
				{
					int between_chess;
					if(row_gap > 0)
						between_chess = chess[from_location_no-4*i] ;
					else
						between_chess = chess[from_location_no+4*i] ;
					if(between_chess != CHESS_EMPTY)
						between_Count++;
				}
				
			}
			
			if(between_Count != 1 )
			// if there are more than two pieces or not any piece between starting point and destination
			{
				// MessageNo = 4;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
			else if(to_chess_no == CHESS_EMPTY)
			{
				// MessageNo = 5;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
		}
		//slide
		else
		{
			// MessageNo = 6;
			//strcat(Message,"**cant slide**");
			IsCurrent = false;
		}
	}
	else // non gun
	{
		//judge pawn or king

		//distance
		if( abs(from_row-to_row) + abs(from_col-to_col)  > 1) // if not legal move
		{
			// MessageNo = 7;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
		//judge pawn
		else if(from_chess_no % 7 == 0)
		{
			if(to_chess_no % 7 != 0 && to_chess_no % 7 != 6)
			{
				// MessageNo = 8;
				//strcat(Message,"**pawn only eat pawn and king**");
				IsCurrent = false;
			}
		}
		//judge king
		else if(from_chess_no % 7 == 6 && to_chess_no % 7 == 0)
		{
			// MessageNo = 9;
			//strcat(Message,"**king can't eat pawn**");
			IsCurrent = false;
		}
		else if(from_chess_no % 7 < to_chess_no% 7)
		{
			// MessageNo = 10;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
	}
	return IsCurrent;
}

int hammingWeight(unsigned int n) {
	int hamWt = 0;
	while (n > 0) {
		hamWt += n & 1; // Check rightmost bit, add to weight if 1
		n = n >> 1;     // Shift bits to the right
	}
	return hamWt;
}

// Evaluating function
// always use my point of view, so use this->Color
double MyAI::Evaluate(const ChessBoard* chessboard, 
	const int legal_move_count, const int color){
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero

	double score = OFFSET;
	bool finish;

	if(legal_move_count == 0){ // Win, Lose
		if(color == this->Color){ // Lose
			score += LOSE - WIN;
		}else{ // Win
			score += WIN - LOSE;
		}
		finish = true;
	}else if(isDraw(chessboard)){ // Draw
		// score += DRAW - DRAW;
		finish = true;
	}else{ // no conclusion
		// static material values
		// cover and empty are both zero
		static const double values[14] = {
			  1,180,  6, 18, 90,270,810,  
			  1,180,  6, 18, 90,270,810
		};

		double piece_value = 0;
		for(int i = 0; i < 32; i++){
			if(chessboard->Board[i] != CHESS_EMPTY && 
				chessboard->Board[i] != CHESS_COVER){
				if(chessboard->Board[i] / 7 == this->Color){
					piece_value += values[chessboard->Board[i]];
				}else{
					piece_value -= values[chessboard->Board[i]];
				}
			}
		}
		// linear map to (-<WIN>, <WIN>)
		// score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
		// <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * (<WIN> - 0.01)
		piece_value = piece_value / 1943 * (WIN - 0.01);
		score += piece_value; 
		finish = false;
	}

	// Bonus (Only Win / Draw)
	if(finish){
		if((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)){
			if(!(legal_move_count == 0 && color == this->Color)){ // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE * 
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if(bonus > BONUS){ bonus = BONUS; } // limit
				score += bonus;
			}
		}else if((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)){
			if(!(legal_move_count == 0 && color != this->Color)){ // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE * 
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if(bonus > BONUS){ bonus = BONUS; } // limit
				score -= bonus;
			}
		}
	}
	
	return score;
}

double MyAI::Nega_max(const ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth, double alpha, double beta){

	int Moves[2048];
	int move_count = 0;

	// move
	// check every possible move and store them in Moves array
	move_count = Expand(chessboard.Board, chessboard.piece, chessboard.color[color], color, Moves);
	// printf("%d\n", depth);

	if(isTimeUp() || // time is up
		remain_depth == 0 || // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count == 0 // terminal node (no move type)
		){
		this->node++;
		// odd: *-1, even: *1
		// printf("e: %f\n", Evaluate(&chessboard, move_count, color) * (depth&1 ? -1 : 1));
		return Evaluate(&chessboard, move_count, color) * (depth&1 ? -1 : 1);
	}else{
		double m = -DBL_MAX;
		int new_move;
		// search deeper
		for(int i = 0; i < move_count; i++){ // move
			ChessBoard new_chessboard = chessboard;
			
			MakeMove(&new_chessboard, Moves[i], 0); // 0: dummy
			// move every possible move
			double t = -Nega_max(new_chessboard, &new_move, color^1, depth+1, remain_depth-1, -beta, - ((m>alpha)?m:alpha));
			// c++ ^ operator is xor (color xor 1)
			if(t > m){ 
				m = t;
				*move = Moves[i];
			// }else if(t == m){
			// 	bool r = rand()%2;
			// 	if(r) *move = Moves[i];
			}
			if (m >= beta) {
				return m;
			}
		}
		return m;
	}
}

bool MyAI::isDraw(const ChessBoard* chessboard){
	// No Eat Flip
	if(chessboard->NoEatFlip >= NOEATFLIP_LIMIT){
		return true;
	}

	// Position Repetition
	int last_idx = chessboard->HistoryCount - 1;
	// -2: my previous ply
	int idx = last_idx - 2;
	// All ply must be move type
	int smallest_repetition_idx = last_idx - (chessboard->NoEatFlip / POSITION_REPETITION_LIMIT);
	// check loop
	while(idx >= 0 && idx >= smallest_repetition_idx){
		if(chessboard->History[idx] == chessboard->History[last_idx]){
			// how much ply compose one repetition
			int repetition_size = last_idx - idx;
			bool isEqual = true;
			for(int i = 1; i < POSITION_REPETITION_LIMIT && isEqual; ++i){
				for(int j = 0; j < repetition_size; ++j){
					int src_idx = last_idx - j;
					int checked_idx = last_idx - i*repetition_size - j;
					if(chessboard->History[src_idx] != chessboard->History[checked_idx]){
						isEqual = false;
						break;
					}
				}
			}
			if(isEqual){
				return true;
			}
		}
		idx -= 2;
	}

	return false;
}

bool MyAI::isTimeUp(){
	double elapsed; // ms
	
	// design for different os
#ifdef WINDOWS
	clock_t end = clock();
	elapsed = (end - begin);
#else
	struct timeval end;
	gettimeofday(&end, 0);
	long seconds = end.tv_sec - begin.tv_sec;
	long microseconds = end.tv_usec - begin.tv_usec;
	elapsed = (seconds*1000 + microseconds*1e-3);
#endif

	this->timeIsUp = (elapsed >= TIME_LIMIT*1000);
	return this->timeIsUp;
}

//Display chess board
void MyAI::Pirnf_Chessboard()
{	
	char Mes[1024] = "";
	char temp[1024];
	char myColor[10] = "";
	if(Color == -99)
		strcpy(myColor, "Unknown");
	else if(this->Color == RED)
		strcpy(myColor, "Red");
	else
		strcpy(myColor, "Black");
	sprintf(temp, "------------%s-------------\n", myColor);
	strcat(Mes, temp);
	strcat(Mes, "<8> ");
	for(int i = 0; i < 32; i++){
		if(i != 0 && i % 4 == 0){
			sprintf(temp, "\n<%d> ", 8-(i/4));
			strcat(Mes, temp);
		}
		char chess_name[4] = "";
		Pirnf_Chess(this->main_chessboard.Board[i], chess_name);
		sprintf(temp,"%5s", chess_name);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n     ");
	for(int i = 0; i < 4; i++){
		sprintf(temp, " <%c> ", 'a'+i);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n");
	printf("%s", Mes);
}


// Print chess
void MyAI::Pirnf_Chess(int chess_no,char *Result){
	// XX -> Empty
	if(chess_no == CHESS_EMPTY){	
		strcat(Result, " - ");
		return;
	}
	// OO -> DarkChess
	else if(chess_no == CHESS_COVER){
		strcat(Result, " X ");
		return;
	}

	switch(chess_no){
		case 0:
			strcat(Result, " P ");
			break;
		case 1:
			strcat(Result, " C ");
			break;
		case 2:
			strcat(Result, " N ");
			break;
		case 3:
			strcat(Result, " R ");
			break;
		case 4:
			strcat(Result, " M ");
			break;
		case 5:
			strcat(Result, " G ");
			break;
		case 6:
			strcat(Result, " K ");
			break;
		case 7:
			strcat(Result, " p ");
			break;
		case 8:
			strcat(Result, " c ");
			break;
		case 9:
			strcat(Result, " n ");
			break;
		case 10:
			strcat(Result, " r ");
			break;
		case 11:
			strcat(Result, " m ");
			break;
		case 12:
			strcat(Result, " g ");
			break;
		case 13:
			strcat(Result, " k ");
			break;
	}
}
