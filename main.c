#include <stdio.h>
#include <stdlib.h>
#include "includes.h"
#include "address_map_nios2.h"
#include "hex_display.h"
#include "altera_avalon_pio_regs.h"
#include "altera_up_avalon_character_lcd.h"


// Constants

#define IDLE	0
#define PLAY	1
#define PAUSE   2

// Mailboxes
OS_EVENT *GametimeMBox;
OS_EVENT *LevelOverMBox;

// Semaphores
OS_EVENT *StateSem;
OS_EVENT *GametimeSem;
OS_EVENT *ElapsedTimeSem;
OS_EVENT *QuestionSem;
OS_EVENT *TotalScoreSem;
OS_EVENT *AnswerSem;
OS_EVENT *LevelSem;
OS_EVENT *Question_CountSem;
OS_EVENT *DispSem;
OS_EVENT *SWSem;
OS_EVENT *REDLEDSem;

/* Errors */
INT8U err_state;
INT8U err_game_time;
INT8U err_elapsed_time;
INT8U err_question;
INT8U err_totalscore;
INT8U err_answer;
INT8U err_level;
INT8U err_questioncount;
INT8U err_disp;
INT8U err_SW;
INT8U err_redLED;

// Global Variables

volatile int state = IDLE;
volatile int game_time = 30;
volatile int elapsed_time = 0;
volatile int question = 0;
volatile int total_score = 0;
volatile int answer = 0;
volatile int Round = 1;
volatile int question_count = 0;
alt_up_character_lcd_dev * char_lcd_dev;

int HEX7_4;
int HEX3_0;
int KEY_ptr;
int red_LED_ptr;
int green_LED_ptr;

char gameTime[5];
char elapsedTime[5];
char totalScore[5];
char Question[5];

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    Timer_task_stk[TASK_STACKSIZE];
OS_STK    Level_over_task_stk[TASK_STACKSIZE];
OS_STK    Key_press_task_stk[TASK_STACKSIZE];
OS_STK    Game_time_task_stk[TASK_STACKSIZE];
OS_STK    SW_state_task_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define Timer_task_PRIORITY      1
#define Key_press_task_PRIORITY      2
#define SW_state_task_PRIORITY      3
#define Game_time_task_PRIORITY      4
#define Level_over_task_PRIORITY      5

void Timer_task(void* pdata)
{

	while (1)
	{
		OSSemPend(SWSem, 0, err_SW);
		int SW_value;
		SW_value = IORD_ALTERA_AVALON_PIO_DATA(SLIDER_SWITCHES_BASE);
		OSSemPost(SWSem);

		int gametimeMbox;
		// If we are in the PLAY State and powered ON edit the timers
		if ( (state == PLAY) && (bit_check(SW_value, 17) == 1) ){

			// Write to elapsed_time
			OSSemPend(ElapsedTimeSem, 0, err_elapsed_time);
			elapsed_time += 1;
			OSSemPost(ElapsedTimeSem);

			// Reduce game time by 1
			OSSemPend(GametimeSem, 0, err_game_time);
			game_time -= 1;
			OSSemPost(GametimeSem);

			// If the game time is up send a message to the GametimeMbox
			if (game_time == 0){
				OSMboxPost(GametimeMBox, (void*)&gametimeMbox);
			}

			// Update the Hex displays
			OSSemPend(DispSem, 0, err_disp);
			HEX7_4 = HEX7_6_display(total_score) + HEX5_4_display(game_time);
			IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);

			// Convert integer to character array
		 	itoa(game_time,gameTime,10);
		 	itoa(total_score,totalScore,10);
		 	itoa(question,Question,10);

			alt_up_character_lcd_init(char_lcd_dev);
			alt_up_character_lcd_string(char_lcd_dev,"left");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,5,0);
			alt_up_character_lcd_string(char_lcd_dev, gameTime);
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
			alt_up_character_lcd_string(char_lcd_dev,"score");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
			alt_up_character_lcd_string(char_lcd_dev, totalScore);
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
			alt_up_character_lcd_string(char_lcd_dev,"question");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,12,1);
			alt_up_character_lcd_string(char_lcd_dev, Question);

			OSSemPost(DispSem);

		}

		OSTimeDlyHMSM(0, 0, 1, 0);
	}

}


void SW_state_task(void* pdata){

	while (1){

		OSSemPend(SWSem, 0, err_SW);
		int SW_value;
		SW_value = IORD_ALTERA_AVALON_PIO_DATA(SLIDER_SWITCHES_BASE);
		OSSemPost(SWSem);


		// Turn off the board when reset happens
		if ( bit_check(SW_value, 17) == 0 ){

			OSSemPend(GametimeSem, 0, err_game_time);
			game_time = 0;
			OSSemPost(GametimeSem);

			OSSemPend(ElapsedTimeSem, 0, err_elapsed_time);
			elapsed_time = 0;
			OSSemPost(ElapsedTimeSem);

			OSSemPend(TotalScoreSem, 0, err_totalscore);
			total_score = 0;
			OSSemPost(TotalScoreSem);

			OSSemPend(QuestionSem, 0, err_question);
			question = 0;
			OSSemPost(QuestionSem);

			OSSemPend(QuestionSem, 0, err_question);
			question_count = 1;
			OSSemPost(QuestionSem);

			OSSemPend(LevelSem, 0, err_level);
			Round = 1;
			OSSemPost(LevelSem);

			OSSemPend(REDLEDSem, 0, err_redLED);
			red_LED_ptr = 0;
			IOWR_ALTERA_AVALON_PIO_DATA(LEDR_BASE, red_LED_ptr);
			OSSemPost(REDLEDSem);

			OSSemPend(StateSem, 0, err_state);
			state = IDLE;
			OSSemPost(StateSem);

			// Both rows in LCD displays blank when SW17 is off
			alt_up_character_lcd_init(char_lcd_dev);
			alt_up_character_lcd_string(char_lcd_dev, "");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
			alt_up_character_lcd_string(char_lcd_dev, "");

			OSSemPend(DispSem, 0, err_disp);
			HEX7_4 = 0;
			IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);
			HEX3_0 = 0;
			IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);

			OSSemPost(DispSem);
		}

		// Display answer through red LED if both system and SW16 is on
		if ( (bit_check(SW_value, 17) == 1) && (bit_check(SW_value, 16) == 1) && (state == PLAY) ){
			OSSemPend(REDLEDSem, 0, err_redLED);
			red_LED_ptr = (SW_value & 0x30000) + question;
			IOWR_ALTERA_AVALON_PIO_DATA(LEDR_BASE, red_LED_ptr);
			OSSemPost(REDLEDSem);
		}


		if ( (state == PAUSE) || (state == IDLE) || ((bit_check(SW_value, 17) == 1) && (bit_check(SW_value, 16) == 0)) ){
			OSSemPend(REDLEDSem, 0, err_redLED);
			red_LED_ptr = 0;
			red_LED_ptr = (SW_value & 0x20000);
			IOWR_ALTERA_AVALON_PIO_DATA(LEDR_BASE, red_LED_ptr);
			OSSemPost(REDLEDSem);
		}


		OSTimeDly(1);
	}
}


// Game time up task
// Checks the Game time task mailbox. If the game time for that level is up move to the next question
// Done
void Game_time_task(void* pdata){

	void *Mbox_accept;
	while (1){

		OSSemPend(SWSem, 0, err_SW);
		int SW_value;
		SW_value = IORD_ALTERA_AVALON_PIO_DATA(SLIDER_SWITCHES_BASE);
		OSSemPost(SWSem);

		Mbox_accept = OSMboxAccept(GametimeMBox);

		if (Mbox_accept != (void*)0){ // If a message is in the Mail box (ie the game time is up)

			// If you run out of time and are ON and in the PLAY state
			if (game_time == 0 && (bit_check(SW_value, 17) == 1) && (state == PLAY)){

				// Reset the time
				OSSemPend(GametimeSem, 0, err_game_time);
				game_time = 30 - (5 * (Round - 1) );
				OSSemPost(GametimeSem);

				// Update the question
				question = random_value(255, 1); // Update the question
				question_count += 1;

				//Update Hex
				OSSemPend(DispSem, 0, err_disp);
				HEX7_4 = HEX7_6_display(total_score) + HEX5_4_display(game_time);
				IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);
				HEX3_0 = HEX2_0_display(question);
				IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);

		 		// Convert integer to character array
		 		itoa(game_time,gameTime,10);
		 		itoa(total_score,totalScore,10);
		 		itoa(question,Question,10);

				alt_up_character_lcd_init(char_lcd_dev);
				alt_up_character_lcd_string(char_lcd_dev,"left");
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,5,0);
				alt_up_character_lcd_string(char_lcd_dev, gameTime);
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
				alt_up_character_lcd_string(char_lcd_dev,"score");
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
				alt_up_character_lcd_string(char_lcd_dev, totalScore);
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
				alt_up_character_lcd_string(char_lcd_dev,"question");
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,12,1);
				alt_up_character_lcd_string(char_lcd_dev, Question);
				OSSemPost(DispSem);

			}
		}

		OSTimeDly(1);
	  }

}


// Level over task
// Checks the LevelOverMBox. If we do 10 questions we will set the game to idle and move to the next level
void Level_over_task(void* pdata){
	void *Mbox_accept;
	while (1)
	{
	  Mbox_accept = OSMboxAccept(LevelOverMBox);
	  if (Mbox_accept != (void*)0){ // If a message is in the Mail box (ie the level is over)
		  //Change the state to IDLE
		  OSSemPend(StateSem, 0, err_state);
		  state = IDLE;
		  OSSemPost(StateSem);

		  //Update Hex
		  OSSemPend(DispSem, 0, err_disp);
		  HEX7_4 = HEX7_6_display(total_score);	// Display the total score
		  IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);
		  HEX3_0 = HEX3_0_display(elapsed_time); // Display the elapsed time for that game
		  IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);
		  OSSemPost(DispSem);

	 	  // Convert integer to character array
	 	  itoa(elapsed_time,elapsedTime,10);
	 	  itoa(total_score,totalScore,10);

		  alt_up_character_lcd_init(char_lcd_dev);
		  alt_up_character_lcd_string(char_lcd_dev,"time");
		  alt_up_character_lcd_set_cursor_pos(char_lcd_dev,4,0);
		  alt_up_character_lcd_string(char_lcd_dev, elapsedTime);
		  alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
		  alt_up_character_lcd_string(char_lcd_dev,"score");
		  alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
		  alt_up_character_lcd_string(char_lcd_dev, totalScore);

		  // reset the question count
		  OSSemPend(Question_CountSem, 0, err_questioncount);
		  question_count = 1;
		  OSSemPost(Question_CountSem);

		  // Increase the round
		  OSSemPend(LevelSem, 0, err_level);
		  Round++;
		  OSSemPost(LevelSem);

		  // Set the Game time
		  OSSemPend(GametimeSem, 0, err_game_time);
		  game_time = 30 - (5 * Round - 1) ; // Formula to add more difficulty -5s after each level
		  OSSemPost(GametimeSem);
		  printf("Level %d\n", Round);
	  }

	  OSTimeDly(1);
  }
}

// Key press task
void Key_press_task(void* pdata)
{

	while (1)
	{
		OSSemPend(SWSem, 0, err_SW);
		int SW_value;
		SW_value = IORD_ALTERA_AVALON_PIO_DATA(SLIDER_SWITCHES_BASE);  // read the SW slider (DIP) switch values
		OSSemPost(SWSem);

		int KEY_press;
		KEY_press = IORD_ALTERA_AVALON_PIO_DATA(KEY_BASE);// read the pushbutton interrupt register
		green_LED_ptr = KEY_press;
		IOWR_ALTERA_AVALON_PIO_DATA(LEDG_BASE, green_LED_ptr); // Light up the green LED when we press 

	    // Get the answer value
	    if ( (bit_check(SW_value, 17) == 1) && (bit_check(SW_value, 16) == 0) ){
			OSSemPend(AnswerSem, 0, err_answer);
	    	answer = SW_value - 131072; // get the answer minus SW17
	    	OSSemPost(AnswerSem);
	    }
	    else if ( (bit_check(SW_value, 17) == 1) && (bit_check(SW_value, 16) == 1) ){
	    	OSSemPend(AnswerSem, 0, err_answer);
	        answer = SW_value - 196608; // get the answer minus SW17 and SW16
	        OSSemPost(AnswerSem);
	    }


	    if (KEY_press & 0x1) { // KEY0
	        if ((state == PAUSE) && (bit_check(SW_value, 17) == 1)) { // If we are in the Pause state and ON, enter IDLE state

	        	OSSemPend(GametimeSem, 0, err_game_time);
	        	 game_time = 30;
	        	OSSemPost(GametimeSem);

				OSSemPend(ElapsedTimeSem, 0, err_elapsed_time);
				elapsed_time = 0;
				OSSemPost(ElapsedTimeSem);

	        	OSSemPend(TotalScoreSem, 0, err_totalscore);
	        	total_score = 0;
	        	OSSemPost(TotalScoreSem);

				OSSemPend(QuestionSem, 0, err_question);
				question = 0;
				OSSemPost(QuestionSem);

				OSSemPend(QuestionSem, 0, err_question);
				question_count = 1;
				OSSemPost(QuestionSem);

				OSSemPend(LevelSem, 0, err_level);
				Round = 1;
				OSSemPost(LevelSem);

	        	OSSemPend(DispSem, 0, err_disp);
		 		HEX7_4 = HEX7_6_display(total_score) ;	// Display the total score and total game time
		 		IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);
		 		HEX3_0 = HEX3_0_display(elapsed_time); // Display the total elapsed time
		 		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);

		 		// Convert integer to character array
		 		itoa(elapsed_time,elapsedTime,10);
		 		itoa(total_score,totalScore,10);

				alt_up_character_lcd_init(char_lcd_dev);
				alt_up_character_lcd_string(char_lcd_dev,"time");
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,4,0);
				alt_up_character_lcd_string(char_lcd_dev, elapsedTime);
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
				alt_up_character_lcd_string(char_lcd_dev,"score");
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
				alt_up_character_lcd_string(char_lcd_dev, totalScore);

				// Display instruction to start the game
			    alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
				alt_up_character_lcd_string(char_lcd_dev, "Press KEY1 to start.....");
	        	OSSemPost(DispSem);

	        	OSSemPend(StateSem, 0, err_state);
	        	state = IDLE;
	        	OSSemPost(StateSem);
	        }
	    }

	    if (KEY_press & 0x2){ // KEY1
	    	if ( (state == IDLE) && (bit_check(SW_value, 17) == 1) ) { // When in IDLE switch to the PLAY State
	    	   printf("PLAY!!!!\n");

	    	   OSSemPend(GametimeSem, 0, err_game_time);
	    	   game_time = 30 - (5 * (Round - 1) );
	    	   OSSemPost(GametimeSem);

	    	   OSSemPend(QuestionSem, 0, err_question);
	    	   question = random_value(255, 1);
	    	   OSSemPost(QuestionSem);

	    	   OSSemPend(DispSem, 0, err_disp);
	 		   HEX3_0 = HEX3_0_display(question); // Display the question
	 		   IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);
	 		   HEX7_4 = HEX7_6_display(total_score) + HEX5_4_display(game_time);	// Display the total score and total game time
	 		   IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);

	 		   // Convert integer to character array
		 	   itoa(game_time,gameTime,10);
		 	   itoa(total_score,totalScore,10);
		 	   itoa(question,Question,10);

			   alt_up_character_lcd_init(char_lcd_dev);
			   alt_up_character_lcd_string(char_lcd_dev,"left");
			   alt_up_character_lcd_set_cursor_pos(char_lcd_dev,5,0);
			   alt_up_character_lcd_string(char_lcd_dev, gameTime);
			   alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
			   alt_up_character_lcd_string(char_lcd_dev,"score");
			   alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
			   alt_up_character_lcd_string(char_lcd_dev, totalScore);
			   alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
			   alt_up_character_lcd_string(char_lcd_dev,"question");
			   alt_up_character_lcd_set_cursor_pos(char_lcd_dev,12,1);
			   alt_up_character_lcd_string(char_lcd_dev, Question);
	    	   OSSemPost(DispSem);

	    	   OSSemPend(StateSem, 0, err_state);
	    	   state = PLAY;
	    	   OSSemPost(StateSem);
	    	}
	    	else if ( (state == PLAY) && (bit_check(SW_value, 17) == 1) ){ // If in play Pause the game
	    	    printf("PAUSE\n");

	    	    OSSemPend(DispSem, 0, err_disp);
		 		HEX3_0 = HEX3_0_display(0); // Hide the question when paused
		 		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);
		 		HEX7_4 = HEX7_6_display(total_score) + HEX5_4_display(game_time);	// Display the total score and total game time
		 		IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);

		 		// Convert integer to character array
		 		itoa(game_time,gameTime,10);
		 		itoa(total_score,totalScore,10);

				alt_up_character_lcd_init(char_lcd_dev);
				alt_up_character_lcd_string(char_lcd_dev,"left");
			    alt_up_character_lcd_set_cursor_pos(char_lcd_dev,5,0);
				alt_up_character_lcd_string(char_lcd_dev, gameTime);
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
				alt_up_character_lcd_string(char_lcd_dev,"score");
			    alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
				alt_up_character_lcd_string(char_lcd_dev, totalScore);

				// Display instruction to resume the game
			    alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
				alt_up_character_lcd_string(char_lcd_dev, "Press KEY1 to resume.....");
	    	    OSSemPost(DispSem);

	    	    OSSemPend(StateSem, 0, err_state);
	    	    state = PAUSE;
	    	    OSSemPost(StateSem);
	    	}
	    	else if ( (state == PAUSE) && (bit_check(SW_value, 17) == 1) ){ // If paused go to play
	    	    printf("PLAY!!!!\n");

	    	    OSSemPend(DispSem, 0, err_disp);
		 		HEX3_0 = HEX2_0_display(question); // Show the question again when non-paused
		 		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);

		 		// Convert integer to character array
		 		itoa(question,Question,10);
				alt_up_character_lcd_init(char_lcd_dev);
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
				alt_up_character_lcd_string(char_lcd_dev,"question");
				alt_up_character_lcd_set_cursor_pos(char_lcd_dev,12,1);
				alt_up_character_lcd_string(char_lcd_dev, Question);
	    	    OSSemPost(DispSem);

	    	    OSSemPend(StateSem, 0, err_state);
	    	    state = PLAY;
	    	    OSSemPost(StateSem);
	    	}
	    }

	    if (KEY_press & 0x4) { // KEY2
	        printf("question: %d\n", question);
	        printf("input: %d\n", answer);
	        // If the answer is correct
	        if ( (answer == question) && (state == PLAY) ){
	        	OSSemPend(TotalScoreSem, 0, err_totalscore);
	            total_score += 1; // Add to the score
	            OSSemPost(TotalScoreSem);
	        }

            if (question_count == 11){// The round is over after 10 questions (start at q=1 so q_c = 11 so 11-1=10)
            	int leveloverMbox;
            	OSMboxPost(LevelOverMBox, (void*)&leveloverMbox);
            }

            // Reset the time
            OSSemPend(GametimeSem, 0, err_game_time);
            game_time = 30 - (5 * (Round - 1) );
            OSSemPost(GametimeSem);


            // Update the question
            OSSemPend(QuestionSem, 0, err_question);
            question = random_value(255, 1); // Update the question
            OSSemPost(QuestionSem);

            OSSemPend(QuestionSem, 0, err_question);
            question_count += 1;
            OSSemPost(QuestionSem);

            OSSemPend(DispSem, 0, err_disp);
	 		HEX7_4 = HEX7_6_display(total_score) + HEX5_4_display(game_time);	// Display the total score and total game time
	 		IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);
	 		HEX3_0 = HEX2_0_display(question); // Display the question
	 		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);


	 		// Convert integer to character array
	 		itoa(game_time,gameTime,10);
	 		itoa(total_score,totalScore,10);
	 		itoa(question,Question,10);

			alt_up_character_lcd_init(char_lcd_dev);
			alt_up_character_lcd_string(char_lcd_dev,"left");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,5,0);
			alt_up_character_lcd_string(char_lcd_dev, gameTime);
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,7,0);
			alt_up_character_lcd_string(char_lcd_dev,"score");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,13,0);
			alt_up_character_lcd_string(char_lcd_dev, totalScore);
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,0,1);
			alt_up_character_lcd_string(char_lcd_dev,"question");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev,12,1);
			alt_up_character_lcd_string(char_lcd_dev, Question);
            OSSemPost(DispSem);

	    }
	    OSTimeDly(1);
	}

}


int main(void) {
	OSInit();
	// Create the mail boxes
	GametimeMBox = OSMboxCreate((void*)0); // Create a mailbox for activating the run out of time task
	LevelOverMBox = OSMboxCreate((void*)0); // Create a mailbox for the Level over task

	// Create Semaphores
	StateSem = OSSemCreate(1);
	GametimeSem = OSSemCreate(1);
	ElapsedTimeSem = OSSemCreate(1);
	QuestionSem = OSSemCreate(1);
	TotalScoreSem = OSSemCreate(1);
	AnswerSem = OSSemCreate(1);
	LevelSem = OSSemCreate(1);
	Question_CountSem = OSSemCreate(1);
	DispSem = OSSemCreate(1);
	SWSem = OSSemCreate(1);
	REDLEDSem = OSSemCreate(1);


	game_time = 30;
	elapsed_time = 0;
	question = 0;
	total_score = 0;
	answer = 0;
	Round = 1;
	question_count = 0;

	printf("Initiating binary game...\n");

	//Open the character LCD port
	char_lcd_dev = alt_up_character_lcd_open_dev("/dev/Char_LCD_16x2");

	if(char_lcd_dev == NULL){
		alt_printf("Error: could not open character LCD device\n");
	}
	else{
		alt_printf("Opened character LCD device\n");
	}

	// Initialize the character display
	alt_up_character_lcd_init(char_lcd_dev);

	// Set All displays to 0
	HEX7_4 = HEX7_6_display(0) + HEX5_4_display(0);
	HEX3_0 = HEX3_0_display(0);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE, HEX7_4);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE, HEX3_0);

	//Timer task
    OSTaskCreateExt(Timer_task,
    		NULL,
    		(void *)&Timer_task_stk[TASK_STACKSIZE-1],
    		Timer_task_PRIORITY,
    		Timer_task_PRIORITY,
			Timer_task_stk,
			TASK_STACKSIZE,
			NULL,
			0);

    // Level Over task
    OSTaskCreateExt(Level_over_task,
    		NULL,(void *)&Level_over_task_stk[TASK_STACKSIZE-1],
			Level_over_task_PRIORITY,
			Level_over_task_PRIORITY,
			Level_over_task_stk,
			TASK_STACKSIZE,
			NULL,
			0);


    //Key press task
    OSTaskCreateExt(Key_press_task,
    		NULL,(void *)&Key_press_task_stk[TASK_STACKSIZE-1],
			Key_press_task_PRIORITY,
    		Key_press_task_PRIORITY,
			Key_press_task_stk,
			TASK_STACKSIZE,
			NULL,
			0);

    // Game time task
    OSTaskCreateExt(Game_time_task,
    		NULL,
			(void *)&Game_time_task_stk[TASK_STACKSIZE-1],
			Game_time_task_PRIORITY,
    		Game_time_task_PRIORITY,
			Game_time_task_stk,
			TASK_STACKSIZE,
			NULL,
			0);

    //SW state task
    OSTaskCreateExt(SW_state_task,
    		NULL,
			(void *)&SW_state_task_stk[TASK_STACKSIZE-1],
			SW_state_task_PRIORITY,
    		SW_state_task_PRIORITY,
			SW_state_task_stk,
			TASK_STACKSIZE,
			NULL,
			0);


    OSStart();
    return 0;
}




