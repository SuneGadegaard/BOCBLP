#include <iostream>
#include <string>
#include<stdexcept>
#include<chrono>
#include<ratio>

#include "BOCBLPsolver.h"

using namespace std::chrono;
typedef std::chrono::high_resolution_clock CPUclock;

// argv[1] = path to a problem data file
/* argv[2] = problem type. If no problemtype is provided we default to p-median
             uncapacitated facility location problem = uflp
             capacitated facility location problem = cflp
             single source capacitated facility location problem = sscflp */

int main( int argc, char** argv )
{
    try{
        if ( argc == 2 )
        {   // Check if a datafile is provided
            throw std::runtime_error ( "You need to specify a data file. Terminating!\n" );
        }

        std::string FileName  = argv[1];
        int ProblemType = 0; // Defaults to zero ( a p-median problem )

        if ( argc > 2 ){

            std::string ptype = argv[2];

            if ( ptype.compare("uflp") ==  0 )
            {
                ProblemType = 1;
            }
            else if ( ptype.compare("cflp") == 0 )
            {
                ProblemType = 2;
            }
            else if ( ptype.compare("sscflp") == 0 )
            {
                ProblemType = 3;
            }
            else
            {
                throw std::runtime_error ( "The specified location problem is unknown to the program! Terminating!\n");
            }
        }

        BOCBLPsolver solver = BOCBLPsolver ( FileName , ProblemType );

        solver.InstanceNumber   = 0; // You can specify an instane id here
        solver.CostMethod       = 0; // You can specify a cost structure id here
        solver.outputfile   = "./" + std::to_string ( solver.InstanceNumber ) + "_" + std::to_string ( solver.CostMethod ) + "_out"; // Specify the name of the output file
        solver.SummaryFile  = "./summary_out"; // Specify the summary file the summary should be added to


        solver.run();

        std::cout << "Total solution time was         : " << solver.getCPUtime() << "\n";
        std::cout << "Number of non--dominated points : " << solver.getFrontierSize() << "\n";

        return 0;
    }
    catch ( std::exception &e )
    {
        std::cerr << "Exception in main : " << e.what ( ) << "\n";
        exit ( EXIT_FAILURE );
    }
}
