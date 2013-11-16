// class for solving SAT-problem using MPI

#ifndef mpi_solver_h
#define mpi_solver_h

#include "mpi_base.h"

class MPI_Solver : public MPI_Base
{
public:
	// Constructor/Destructor:
    MPI_Solver( );
    ~MPI_Solver( );
 
	int extra_tasks_count; // count of cnf that can additionally be doubled
	int orig_tasks_count;
	int full_mask_tasks_count;
	int exch_activ;
	int skip_tasks;
	string solving_info_file_name;

	double *solving_times;
	vector<double> total_solving_times;
	
	bool MPI_Solve( int argc, char **argv );
	bool MPI_ConseqSolve( int argc, char **argv );

	bool WriteTimeToFile( double whole_time_sec );
	bool cpuTimeInHours( double full_seconds, int &real_hours, int &real_minutes, 
		                 int &real_seconds );

	bool ControlProcessSolve( int first_range_tasks_count, unsigned *full_mask_ext, 
		                      unsigned **values_arr );
	bool ComputeProcessSolve( );

	bool GetExtraTasks( unsigned **&values_arr, unsigned *full_mask_ext );

	void PrintParams( );

	void WriteSolvingTimeInfo( double *solving_times, vector<double> total_solving_times, 
							   unsigned solved_tasks_count, unsigned sat_count, 
							   double finding_first_sat_time );

	void AddSolvingTimeToArray( ProblemStates cur_problem_state, double cnf_time_from_node, 
		                        double *solving_times );

	bool SolverRun( Solver *&S, int &process_sat_count, double &cnf_time_from_node, 
				    int current_task_index );
};

#endif
