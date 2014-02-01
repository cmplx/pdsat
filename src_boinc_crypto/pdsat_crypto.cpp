// BOINC client application PDSAT

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cstdio>
#include <cctype>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#endif

#include "str_util.h"
#include "util.h"
#include "filesys.h"
#include "boinc_api.h"
#include "mfile.h"

#include "minisat22_wrapper.h"

using namespace std;

#define CHECKPOINT_FILE "chpt"
#define INPUT_FILENAME "in"
#define OUTPUT_FILENAME "out"

const int MAX_NOF_RESTARTS            = 5000;
const int MIN_CHECKPOINT_INTERVAL_SEC = 10;

int last_iteration_done = 0;
int total_problems_count = 0;
string previous_results_str = "";

int bivium_0_cnf_array[] = {
#include "bivium_test_0.inc"
};

int bivium_1_cnf_array[] = {
#include "bivium_test_1.inc"
};

int bivium_2_cnf_array[] = {
#include "bivium_test_2.inc"
};

bool do_work( FILE *infile, string &final_result_str );
int do_checkpoint( unsigned current_solved, unsigned total_tasks, string &final_result_str );

int main(int argc, char **argv) {
    int c, nchars = 0, retval, n;
    double fsize;
    char input_path[512], output_path[512], chkpt_path[512], buf[256];
    MFILE outfile;
    FILE *chpt_file, *infile;

    retval = boinc_init();
    if (retval) {
        fprintf(stderr, "%s boinc_init returned %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(retval);
    }

    // open the input file (resolve logical name first)
    boinc_resolve_filename( INPUT_FILENAME, input_path, sizeof(input_path) );
    infile = boinc_fopen(input_path, "r");
    if (!infile) {
        fprintf(stderr,
            "%s Couldn't find input file, resolved name %s.\n",
            boinc_msg_prefix(buf, sizeof(buf)), input_path
        );
        exit(-1);
    }

    // get size of input file (used to compute fraction done)
    file_size( input_path, fsize );

	// resolve output file
    boinc_resolve_filename( OUTPUT_FILENAME, output_path, sizeof(output_path) );
	
	// See if there's a valid checkpoint file.
	char string_input[1024];
    boinc_resolve_filename( CHECKPOINT_FILE, chkpt_path, sizeof(chkpt_path) );
    chpt_file = boinc_fopen(chkpt_path, "r");
	
    if ( chpt_file ) {
        n = fscanf( chpt_file, "%d %d", &last_iteration_done, &total_problems_count );
		while ( fgets(string_input, 1024, chpt_file ) )
			previous_results_str += string_input;
        fclose( chpt_file );
    }

	retval = outfile.open(output_path, "w");
    if (retval) {
        fprintf(stderr, "%s APP: app output open failed:\n",
            boinc_msg_prefix(buf, sizeof(buf))
        );
        fprintf(stderr, "%s resolved name %s, retval %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), output_path, retval
        );
        perror("open");
        exit(1);
    }

	string final_result_str;
	if ( !do_work( infile, final_result_str ) ) {
		fprintf( stderr, "%s APP: do_work() failed:\n" );
		perror("do_work");
        exit(1);
	}

	outfile.puts( final_result_str.c_str() );
    retval = outfile.flush();
    if (retval) {
        fprintf( stderr, "%s APP: failed %d\n",
                 boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(1);
    }

    boinc_finish(0);
}

bool do_work( FILE *infile, string &final_result_str )
{
	int retval;
	string error_msg;
	
	/*if ( !ls.ReadLiteralsFromFile( infile, error_msg ) ) {
		fprintf(stderr, "APP: error in ls.ReadLiteralsFromFile\n");
		fprintf(stderr, "APP: %s\n", error_msg.c_str());
		return false;
	}

	if ( total_problems_count == 0 ) // if first launch and no chkpt file
		total_problems_count = ls.positive_literals.size();
	
	if ( ls.problem_type.find("inc") != string::npos ) {
		ls.cnf_array.resize( sizeof(LS_10_3_inc72_cnf_array)  / sizeof(LS_10_3_inc72_cnf_array[0]) );
		for ( unsigned i = 0; i < ls.cnf_array.size(); i++ ) 
			ls.cnf_array[i] = LS_10_3_inc72_cnf_array[i];
		fprintf(stderr, ls.problem_type.c_str() );
	}
	*/

	minisat22_wrapper m22_wrapper;
	Problem cnf;
	//m22_wrapper.parse_DIMACS_from_inc( cnf_array, cnf );
	Solver *S;

	string current_result_str = "";
	vector< vector<int> > :: iterator positive_literals_it;
	double current_time = 0, time_last_checkpoint = 0;
	clock_t clk_start = clock();
	time_last_checkpoint = (double)(clock( ) - clk_start)/(double)(CLOCKS_PER_SEC);
	
	S = new Solver();
	S->max_nof_restarts = MAX_NOF_RESTARTS;
	fprintf( stderr, " %d ", S->max_nof_restarts );
	S->addProblem( cnf ); // add initial CNF every time

	/*for ( positive_literals_it = ls.positive_literals.begin() + last_iteration_done; 
		  positive_literals_it != ls.positive_literals.end(); positive_literals_it++ ) 
	{
		// solve

		current_time = (double)(clock( ) - clk_start)/(double)(CLOCKS_PER_SEC);
		// skip some time in case of fast checkpoint to make it correct
		if ( current_time >= time_last_checkpoint + MIN_CHECKPOINT_INTERVAL_SEC ) {
			time_last_checkpoint = current_time;
			//current_result_str = previous_results_str + ;
			// checkpoint current position and results
			//if ( ( boinc_is_standalone() ) || ( boinc_time_to_checkpoint() ) ) {
				retval = do_checkpoint( ls.problems_solved + last_iteration_done, total_problems_count, current_result_str );
				if (retval) {
					fprintf(stderr, "APP: checkpoint failed %d\n", retval );
					exit(retval);
				}
				boinc_checkpoint_completed();
			//}
		}
	}*/

	delete S;

	final_result_str = current_result_str;
	
	return true;
}

int do_checkpoint( unsigned current_solved, unsigned total_tasks, string &current_result_str ) {
    int retval;
    string resolved_name;

    FILE* f = fopen("temp", "w");
    if (!f) return 1;
    fprintf( f, "%d %d \n%s", current_solved, total_tasks, current_result_str.c_str() );
    fclose( f );

    boinc_resolve_filename_s(CHECKPOINT_FILE, resolved_name);
    retval = boinc_rename( "temp", resolved_name.c_str() );
    if ( retval ) return retval;

	boinc_fraction_done( ( double )current_solved / ( double )total_tasks );

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR Args, int WinMode) {
    LPSTR command_line;
    char* argv[100];
    int argc;

    command_line = GetCommandLine();
    argc = parse_command_line( command_line, argv );
    return main(argc, argv);
}
#endif