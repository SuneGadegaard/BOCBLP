#ifndef PURECPLEX_H_INCLUDED
#define PURECPLEX_H_INCLUDED


#include<ilcplex/ilocplex.h>	// Header for IloCplex.
#include<exception>		        // Header for exceptions.
#include<stdexcept>	    	    // Header for exceptions (runtime_error, logical_error a.s.o).
#include<vector>                // Header for the vector class.
#include<algorithm>             // Header for the agorithm class.
#include<random>                // Header for the random class. Provides a long list of random number generators.
#include<chrono>                // Header providing very precise clock
#include<ratio>
#include<fstream>               // Read and write files

#include"rdDat.h"               // Header file for the data reader


typedef IloArray<IloNumVarArray> IloVarMatrix;  //!< An IloArray of IloNumVarArrays
typedef std::chrono::high_resolution_clock CPUclock;
class BOCBLPsolver
{
private:

    int ProblemType;                                //!< 0 = p-median, 1 = CFLP, 2 = UFLP, 3 = SSCFLP

    // Gathering data
    std::vector<std::pair<double,int>> Frontier;	//! Vector of pairs containing the frontier
    std::vector<std::pair<double,int>> WeakPoints;	//! Vector of pairs containing the weakly domoniated points and the frontier (Superset of Frontier)
    std::vector<double> Times;                      //! Vector containing all the execution time of each problem Times[t] holds the time of the problem resulting in point WeakPoints[t]
    int NumOfPP;                                    //! The size of Fontier. That is NumOfPP = Frontier.size ( )
    int NumOfWP;                                    //! The size of WeakPoints. That is NumOfWP = WeakPoints.size ( )
    double TotalTime;                               //! Contains the total execution time of the whole algorithm

    // Functions
    /*!
     * Build the location model specified by the integer ProblemType (can be either 0, 1, 2, or 3)
     */
    void BuildModel();

    /*!
     *  Evaluates the bottleneck cost of the current solution.
     */
    int EvaluateCenter();
public:

    /**
     * @name Cplex
     * This section contains all the cplex gear needed for the algorithm to run.
     */
    ///@{
    IloEnv env;         //!< The Ilo environment used throughout the lifetime of the object
    IloModel model;     //!< The IloModel used to build the model
    IloCplex cplex;     //!< The IloCplex environmnet used to solve the model
    IloNumVarArray y;   //!< The location variables
    IloVarMatrix x;     //!< The assignment variables
    IloObjective OBJ;   //!< The IloObjective extractable used to hold the objective function.
    ///@}

    /**
     * @name Data
     * This section contains all the data for describing the location problems.
     */
    ///@{
    int n;          //!< Number of facilities
    int m;          //!< Number of customrs
    int* d;         //!< Demands
    int* s;         //!< Capacities
    int* f;         //!< Fixed opening cost
    double** c;     //!< Travel cost
    int** t;        //!< Travel time
    int p;          //!< Number of open facilies. Used when the problem is of the p-median type
    int TD;         //!< Total demand. That is TD = \sum_{j\in J} d_j
    ///@}

    unsigned long NrNodes;      //!< Number of branching nodes. Only used when using the lexicographic branch and bound
    double MedianObjective;     //!< Holds the current best value of the min-cost objective when using lexicographic branch and bound
    double CenterObjective;     //!< Holds the value of the bottleneck objective of the current best solution to the min-cost problem when using lexicographic branch and bound
    std::string outputfile;     //!< File used to write info of the solution process to. Overwrites content if file already exists
    std::string SummaryFile;    //!< File used to a sumary data to. Appends to the file if it already exists.
    int InstanceNumber;         //!< Holds the id-number of the current data instance. It is used to name the outputfile as well as the corresponding line in the summaryfile
    int CostMethod;             //!< Holds the id-number of the current cost stucture. It is used to name the outputfile as well as the corresponding line in the summaryfile

    /*!
     * The constructor of the class. Takes a data--file and reads the data using rdDat classed defined in rdDat.h and an integer specifying the problem type.
     * \param DataFile  string. Must contain the address of a valid data file
     * \param TheProblemType integer. Integer between 0 and 3 specifying the class of location problem you want to solve. 0 = p-median, 1 = CFLP, 2 = UFLP, 3 = SSCFLP
     */
    BOCBLPsolver ( std::string DataFile, int TheProblemType );

    /*!
     * The destructor of the class. Releases the memory allocated to cplex and internal data--structures.
     */
    ~BOCBLPsolver();

    /*!
     * Function working as the API for the user.
     */
    void run();
};

#endif // PURECPLEX_H_INCLUDED
