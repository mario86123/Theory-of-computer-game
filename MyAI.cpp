#include "float.h"
#include "time.h"
#include "MyAI.h"

#define TIME_LIMIT 8

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
	begin = clock();
	// iterative-deeping, start from 3, time limit = <TIME_LIMIT> sec
	// why depth start at 3 ??
	for(int depth = 3; (double)(clock() - begin) / CLOCKS_PER_SEC < TIME_LIMIT; depth+=2){
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
	int src = move/100, dst = move%100, idx = 0;
	if(src == dst){ // flip
		chessboard->Board[src] = chess;
		chessboard->CoverChess[chess]--;
		chessboard->NoEatFlip = 0;

		idx = ConvertChessToPiecePosIndex(chess);
		// already have same character piece
		while (chessboard->piece_pos[idx] >= 0) { idx--; }
		chessboard->piece_pos[idx] = src;
	}else { // move
		if(chessboard->Board[dst] != CHESS_EMPTY){ // attack
			if(chessboard->Board[dst] / 7 == 0){ // red
				(chessboard->Red_Chess_Num)--;
			}else{ // black
				(chessboard->Black_Chess_Num)--;
				 // red eaten
			}
			chessboard->NoEatFlip = 0;

			// search for eaten piece
			idx = ConvertChessToPiecePosIndex(chessboard->Board[dst]);
			while (chessboard->piece_pos[idx] != dst) { idx--; }
			chessboard->piece_pos[idx] = -5;
		}else{ // not attack
			chessboard->NoEatFlip += 1;
		}
		chessboard->Board[dst] = chessboard->Board[src];
		idx = ConvertChessToPiecePosIndex(chessboard->Board[src]);
		while (chessboard->piece_pos[idx] != src) { idx--; }
		chessboard->piece_pos[idx] = dst;
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

// board[a[XXX]] may be out of bounds
void sort_four(const int* board, int a[]) {
    int large_1, large_2, small_1, small_2;
    if (board[a[0]] > board[a[1]]) {
        large_1 = a[0];
        small_1 = a[1];
    } else {
        large_1 = a[1];
        small_1 = a[0];
    }
    
    if (board[a[2]] > board[a[3]]) {
        large_2 = a[2];
        small_2 = a[3];
    } else {
        large_2 = a[3];
        small_2 = a[2];
    }
    
    if (board[large_1] > board[large_2]) {
        a[0] = large_1;
        if (board[small_1] > board[small_2]) {
            a[3] = small_2;
            if (board[large_2] > board[small_1]) {
                a[1] = large_2;
                a[2] = small_1;
            } else { // large_2 < small_1 && large_1 > large_2
                a[2] = large_2;
                a[1] = small_1;
            }
        } else {     // small_1 < small_2 && large_1 > large_2
            a[1] = large_2;
            a[2] = small_2;
            a[3] = small_1;
        }
    } else { // large_1 < large_2
        a[0] = large_2;
        if (board[small_1] < board[small_2]) { // small_1 < small_2 && large_1 < large_2
            a[3] = small_1;
            if (board[large_1] > board[small_2]) {
                a[1] = large_1;
                a[2] = small_2;
            } else {
                a[2] = large_1;
                a[1] = small_2;
            }
        } else {     // small_1 > small_2 && large_1 < large_2
            a[1] = large_1;
            a[2] = small_1;
            a[3] = small_2;
        }
    }
}

int MyAI::Expand(const int* board, const int* piece, const int color,int *Result)
{
	int ResultCount = 0;
	// king's character_num = 15 or 31
	for (int i = 15; i >= 15 && piece[color * 16 + i] >= 0; i--) {
		int character_num = color * 16 + i;
		int Move[4] = {piece[character_num] - 4, piece[character_num] + 1,
					   piece[character_num] + 4, piece[character_num] - 1};
		// sort_four(board, Move);
		for(int k=0; k<4;k++)
		{
			if(Move[k] >= 0 && Move[k] < 32 && Referee(board,piece[character_num],Move[k],color))
			{
				Result[ResultCount] = piece[character_num]*100+Move[k];
				ResultCount++;
			}
		}
	}

 	// start from my guard, i initialize to 14 if my color is red, else i initialize to 30
	for (int i = 14; i >= 5; i--) {
		int character_num = color * 16 + i;

		//Gun
		if(character_num % 16 == 5 || character_num % 16 == 6)
		{
			int row = piece[character_num]/4;
			int col = piece[character_num]%4;
			for(int rowCount=row*4;rowCount<(row+1)*4;rowCount++) // same row
			{
				if(Referee(board,piece[character_num],rowCount,color))
				{
					Result[ResultCount] = piece[character_num]*100+rowCount;
					ResultCount++;
				}
			}
			for(int colCount=col; colCount<32;colCount += 4) // same column
			{
				if(Referee(board,piece[character_num],colCount,color))
				{
					Result[ResultCount] = piece[character_num]*100+colCount;
					ResultCount++;
				}
			}
		}
		// important here (move ordering)
		else // if my piece is not a gun
		{
			int Move[4] = {piece[character_num]-4,piece[character_num]+1,piece[character_num]+4,piece[character_num]-1}; // down, right, up, right
			// sort_four(board, Move);
			for(int k=0; k<4;k++)
			{
				if(Move[k] >= 0 && Move[k] < 32 && Referee(board,piece[character_num],Move[k],color))
				{
					Result[ResultCount] = piece[character_num]*100+Move[k];
					ResultCount++;
				}
			}
		}
	}

	// pawn's character_num = 0,1,2,3,4 or 16,17,18,19,20
	for (int i = 4; i >= 0 && piece[color * 16 + i] >= 0; i--) {
		int character_num = color * 16 + i;
		int Move[4] = {piece[character_num] - 4, piece[character_num] + 1,
					   piece[character_num] + 4, piece[character_num] - 1}; // down, right, up, right
		// sort_four(board, Move);
		for(int k=0; k<4;k++)
		{
			if(Move[k] >= 0 && Move[k] < 32 && Referee(board,piece[character_num],Move[k],color))
			{
				Result[ResultCount] = piece[character_num]*100+Move[k];
				ResultCount++;
			}
		}
	}
	
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
		static const double val_for_king[7] = {11000, 6000, 4000, 3000, 2300, 2000, 1500};
		static const double val_not_king[10] = {11000, 4900, 2400, 1100, 500, 240, 110, 50, 20 ,8};
		static const double	static_val[7] = {11000, 4900, 2400, 1100, 500, 3000, 50};
		// bool piece[32] = {0};
		int pawn_count[2] = {0}; // pawn_count[color]
		double piece_value = 0;
		// king's piece value
		if (chessboard->piece_pos[color * 16 + 15] != -5 && chessboard->piece_pos[(color^1) * 16 + 15] != -5) { 
			// if my king still alive and opp's king also alive
			piece_value += (val_for_king[pawn_count[(color^1)]+1] + static_val[0]);
			piece_value -= (val_for_king[pawn_count[color]+1] + static_val[0]);
		} else if (chessboard->piece_pos[color * 16 + 15] != -5 && chessboard->piece_pos[(color^1) * 16 + 15] == -5) {
			// if my king still alive and opp's king died
			piece_value += (val_for_king[pawn_count[(color^1)]] + static_val[0]);
		} else if (chessboard->piece_pos[color * 16 + 15] == -5 && chessboard->piece_pos[(color^1) * 16 + 15] != -5) {
			// if my king died and opp's king still alive
			piece_value -= (val_for_king[pawn_count[color]] + static_val[0]);
		}

		int bigger_enemy_count[2] = {0};
		for (int i = 14; i >= 5; i--) { // start from guard
			if (chessboard->piece_pos[color * 16 + i] != -5 && chessboard->piece_pos[(color^1) * 16 + i] != -5) { 
				// if my guard still alive and opp's guard also alive
				bigger_enemy_count[(color^1)]++;
				bigger_enemy_count[color]++;
				piece_value += (val_not_king[bigger_enemy_count[color] + int(chessboard->piece_pos[(color^1) * 16 + 15] != -5)] + static_val[(i-1)/2-1]);
				piece_value -= (val_not_king[bigger_enemy_count[(color^1)] + int(chessboard->piece_pos[color * 16 + 15] != -5)] + static_val[(i-1)/2-1]);
			} else if (chessboard->piece_pos[color * 16 + i] != -5 && chessboard->piece_pos[(color^1) * 16 + i] == -5) {
				// if my guard still alive and opp's guard died
				bigger_enemy_count[(color^1)]++;
				piece_value += (val_not_king[bigger_enemy_count[color] + int(chessboard->piece_pos[(color^1) * 16 + 15] != -5)] + static_val[(i-1)/2-1]);
			} else if (chessboard->piece_pos[color * 16 + i] == -5 && chessboard->piece_pos[(color^1) * 16 + i] != -5) {
				// if my guard died and opp's guard still alive
				bigger_enemy_count[color]++;
				piece_value -= (val_not_king[bigger_enemy_count[(color^1)] + int(chessboard->piece_pos[color * 16 + 15] != -5)] + static_val[(i-1)/2-1]);
			}
		}
		// pawn's piece value

		// if opp's king still alive
		if (chessboard->piece_pos[(color^1) * 16 + 15] != -5) {
			// my pawn's value
			piece_value += val_not_king[bigger_enemy_count[color]] * pawn_count[color];
		} else { // if opp's king died
			piece_value += val_not_king[9] * pawn_count[color];
		}
		// if my king still alive
		if (chessboard->piece_pos[color * 16 + 15] != -5) {
			// opp's pawn's value
			piece_value -= val_not_king[bigger_enemy_count[(color^1)]] * pawn_count[(color^1)];
		} else { // if my king died
			piece_value -= val_not_king[9] * pawn_count[(color^1)];
		}

		// linear map
		piece_value = piece_value / 165000 * (WIN - 0.01);
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
	move_count = Expand(chessboard.Board, chessboard.piece_pos, color, Moves);

	if(isTimeUp() || // time is up
		remain_depth == 0 || // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count == 0 // terminal node (no move type)
		){
		this->node++;
		// odd: *-1, even: *1
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
			}else if(t == m){
				bool r = rand()%2;
				if(r) *move = Moves[i];
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
	this->timeIsUp = ((double)(clock() - begin) / CLOCKS_PER_SEC >= TIME_LIMIT);
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

