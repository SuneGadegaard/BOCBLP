#include"PureCplex.h"

const bool DoLexiBrancing = false;  // Can be changed to true if you want to perform lexicographic branch and bound
const double tol = 1E-6;
const bool fToZero = false;  // If true, the fixed opening cost is set to zero, and all y_i's are fixed to one!

using namespace std::chrono;

ILOBRANCHCALLBACK1 ( lexibrancher, BOCBLPsolver&, lm )
{
    try
    {
        IloEnv env = lm.env;
        IloNum Incumbent = lm.MedianObjective;
        IloNum nodeLB = getObjValue();
        IloNum nodeDiff = Incumbent -nodeLB;
        IloInt n = lm.n, m = lm.m;

        // Check if we can prune the node!
        if ( Incumbent + tol < nodeLB ) prune();

        // If nodeLB = Incumbent we need to lexibranch. Due to integral data, we say nodeLB = Incumbet if (Incumbent-nodeLB <0.9)
        if ( IloAbs ( nodeDiff ) < 0.9 )
        {
            IloNum CenterObjective = lm.CenterObjective;
            IloNumVarArray BranchVar ( env );
            IloNumArray zero ( env );
            IloCplex::BranchDirectionArray dir ( env );
            IloNumArray ys = IloNumArray ( lm.env );
            getValues ( ys,lm.y );
            IloNumArray xs = IloNumArray ( lm.env );
            // Now we need to know if the point is integer feasible:
            bool isIntegerFeasible = true;

            for ( int i=0; i<n ; ++i )
            {
                if ( ( ys[i]>0.00001 ) && ( ys[i]<0.99999 ) ) isIntegerFeasible = false;

                // If we are still integer feasible, check the x variables
                if ( isIntegerFeasible )
                {
                    getValues ( xs,lm.x[i] );

                    for ( int j=0; j<m; ++j )
                    {
                        if ( xs[j]>0.00001 && xs[j]<0.99999 ) isIntegerFeasible = false;

                        if ( lm.t[i][j] >= CenterObjective )
                        {
                            // If t[i][j]>= CenterObjective, we need to fix that bugger to zero
                            BranchVar.add ( lm.x[i][j] );
                            zero.add ( 0.0 );
                            dir.add ( IloCplex::BranchDown );
                        }
                    }
                }
            }

            if ( isIntegerFeasible )
            {
                // If the node is integer feasible, we only need to  fix a lot of x[i][j] variables to zero!
                makeBranch ( BranchVar, zero, dir, nodeLB );
            }
            else
            {
                // If the node is not integer feasible, we find a fractional variable and branch on it!
                IloNum PsCost, MaxPsCostY=-1.0, MaxPsCostX = -1.0;
                int bestI=0, bestJi=0, bestJj=0;

                for ( int i=0; i<n; ++i )
                {
                    if ( ( ys[i]>0.00001 ) && ( ys[i]<0.99999 ) )
                    {
                        PsCost = IloMax ( getUpPseudoCost ( lm.y[i] ) + getDownPseudoCost ( lm.y[i] ) , 0.0 );

                        if ( PsCost > MaxPsCostY )
                        {
                            MaxPsCostY = PsCost;
                            bestI = i;
                        }
                    }

                    if ( MaxPsCostY >= 0.0 )
                    {
                        // Only look for x variables to branch on, if we have no Y variable to branch on
                        getValues ( xs,lm.x[i] );

                        for ( int j=0; j<m; ++j )
                        {
                            if ( ( xs[j]>0.00001 ) && ( xs[j]<0.99999 ) )
                            {
                                PsCost = IloMax ( getUpPseudoCost ( lm.x[i][j] )+getDownPseudoCost ( lm.x[i][j] ) , 0.0 );

                                if ( PsCost> MaxPsCostX )
                                {
                                    MaxPsCostX = PsCost;
                                    bestJi = i;
                                    bestJj = j;
                                }
                            }
                        }
                    }
                }

                if ( MaxPsCostY > 0.0 )
                {
                    // We found a y variable to branch on. It is bestI, by the way
                    BranchVar.add ( lm.y[bestI] );
                }
                else
                {
                    // We branch on an x variable instead. That is x[bestJi][bestJj] by the way
                    BranchVar.add ( lm.x[bestJi][bestJj] );
                }

                zero.add ( 0.0 );
                dir.add ( IloCplex::BranchDown );
                makeBranch ( BranchVar, zero, dir, nodeLB );
                zero[zero.getSize()-1] = 1.0;
                dir[dir.getSize()-1] = IloCplex::BranchUp;
                makeBranch ( BranchVar, zero, dir, nodeLB );
            }

            BranchVar.end();
            zero.end();
            dir.end();
            ys.end();
            xs.end();
            return;
        }

        // If non of the above cases happen, just let cplex decide on its own!
    }
    catch ( std::exception&e )
    {
        std::cerr << "std::exception in lexibrancher: " << e.what() << std::endl;
    }
    catch ( IloException &ie )
    {
        std::cerr << "IloException in lexibrancher: " << ie.getMessage() << std::endl;
    }
}


ILOINCUMBENTCALLBACK1 ( IncUpdate, BOCBLPsolver&, lm )
{
    try
    {
        IloNum IncumbentMed     = lm.MedianObjective;
        IloNum IncumbentCent    = lm.CenterObjective;

        if ( ( lm.NrNodes%100 ) == 0 ) std::cout << "Objective function valye : " << IncumbentMed << std::endl;

        IloNum Prospect = getObjValue();

        if ( Prospect <= IncumbentMed )
        {
            int n = lm.n, m = lm.m;
            int bestT, ProspectT = 0;

            for ( int j=0; j<m; ++j )
            {
                bestT = INT_MAX;

                for ( int i=0; i<n; ++i )
                {
                    if ( ( getValue ( lm.x[i][j] ) >0.5 ) && ( bestT > lm.t[i][j] ) ) bestT = lm.t[i][j];
                }

                if ( bestT > ProspectT ) ProspectT = bestT;
            }

            std::cout << "New T value : " << ProspectT << std::endl;
            bool improve = ( ( Prospect < IncumbentMed ) || ( ProspectT < IncumbentCent ) );

            if ( improve )
            {
                lm.MedianObjective = Prospect;
                lm.CenterObjective = ProspectT;
            }
        }

        reject();
    }
    catch ( std::exception &e )
    {
        std::cerr << "std::exception in IncUpdater : " << e.what() << std::endl;
    }
    catch ( IloException &ie )
    {
        std::cerr << "IloException in IncUpdater : " << ie.getMessage() << std::endl;
    }
}

ILONODECALLBACK1 ( NodeCount, PureCplex&, lm )
{
    ++lm.NrNodes;
}

ILOMIPCALLBACK1 ( Terminator, BOCBLPsolver&, lm )
{
    IloNum lb = getBestObjValue();
    IloNum Incumbent = lm.MedianObjective;

    if ( lb >= Incumbent + tol ) abort();
}

PureCplex::BOCBLPsolver ( std::string DataFile, int TheProblemType ) :NumOfPP ( 0 ),NumOfWP ( 0 ),NrNodes ( 0 )
{
    try
    {
        double ToD;
        double ToC;
        double DtCcap;
        n = 0;
        m = 0;
        p = 0;
        TD = 0;
        d = nullptr;
        s = nullptr;
        f = nullptr;
        c = nullptr;
        t = nullptr;
        //std::cout << "THE PROBLEM TYPE : " << TheProblemType << std::endl;
        ProblemType = TheProblemType;
        rdDat reader = rdDat ( DataFile, TheProblemType );
        n = reader.getNumFac();
        m = reader.getNumCust();
        c = new double*[n];
        t = new int*[n];

        // All problem types need assignment cost and travel cost!
        for ( int i=0; i<n; ++i )
        {
            c[i] = new double[m];
            t[i] = new int[m];

            for ( int j=0; j<m; ++j )
            {
                c[i][j] = reader.getC ( i,j );
                t[i][j] = reader.getT ( i,j );
            }
        }

        if ( 0 == ProblemType ) //Pmedian
        {
            p = reader.getP();
        }
        else   // UFLP, CFLP or SSCFLP. All need fixed opening costs
        {
            f = new int[n];

            for ( int i=0; i<n; ++i )
            {
                if ( fToZero ) f[i] = 0.0;
                else f[i] = reader.getF ( i );
            }

            if ( 2 <= ProblemType ) // CFLP or SSCFLP. Both need demands and capacities
            {
                d = new int[m];
                s = new int[n];

                for ( int j=0; j<m; ++j )
                {
                    d[j] = reader.getD ( j );
                    TD+=double ( d[j] );
                }

                for ( int i=0; i<n; ++i )
                {
                    s[i] = reader.getS ( i );
                    ToC+=double ( s[i] );
                }
            }

            ToD = double ( TD );
            DtCcap = double ( double ( ToC ) /double ( TD ) );
        }
    }
    catch ( std::exception &e )
    {
        std::cerr << "Exception in the constructore of the class : " << e.what() << std::endl;
        exit ( EXIT_FAILURE );
    }
}

//========================================================================================================//
BOCBLPsolver::~BOCBLPsolver()
{
    IloEnv env;
    IloModel model;
    IloCplex cplex;
    IloNumVarArray y;
    IloVarMatrix x;

    if ( d!=nullptr ) delete[] d;

    if ( s!=nullptr ) delete[] s;

    if ( f!=nullptr ) delete[] f;

    if ( c!=nullptr )
    {
        if ( c[0] ) for ( int i=0; i<n; ++i ) delete[] c[i];

        delete[] c;
    };

    if ( t!=nullptr )
    {
        if ( t[0] ) for ( int i=0; i<n; ++i ) delete[] t[i];

        delete[] t;
    }

    if ( y.getImpl() ) y.end();

    if ( x.getImpl() )
    {
        if ( x[0].getImpl() ) for ( int i=0; i<n; ++i ) x[i].end();

        x.end();
    }

    if ( cplex.getImpl() ) cplex.end();

    if ( model.getImpl() ) model.end();

    env.end();
}

//========================================================================================================//
void BOCBLPsolver::run()
{
    try
    {
        int  centobj;
        double medobj;
        BuildModel();
        cplex.setParam ( IloCplex::MIPSearch,1 );

        if ( DoLexiBrancing )
        {
            cplex.use ( lexibrancher ( env,*this ) );
            cplex.use ( IncUpdate ( env,*this ) );
            cplex.use ( Terminator ( env,*this ) );
            cplex.use ( NodeCount ( env,*this ) );
        }

        // Timer stuff
        CPUclock::time_point OverAllStart;
        CPUclock::time_point Start;
        CPUclock::time_point OverAllEnd;
        CPUclock::time_point End;
        duration<double> single_time;
        duration<double> overall_time;
        double TotalTime;
        OverAllStart = CPUclock::now();
        Start = CPUclock::now();
        // Tell CPLEX not to print to the screen
        cplex.setOut ( env.getNullStream ( ) );
        cplex.setWarning ( env.getNullStream ( ) );

        // Continue as long as CPLEX finds solutions
        while ( cplex.solve() )
        {
            End = CPUclock::now();
            // Evaluate the costs of the solution
            medobj = cplex.getObjValue();
            centobj = EvaluateCenter();
            // Push back the point ot the set of WeakPoints
            WeakPoints.push_back ( std::pair<double,int> ( medobj,  centobj ) );

            //Fix the variables with t[i][j]>=centobj
            for ( int i=0; i<n; ++i )
            {
                for ( int j=0; j<m; ++j )
                {
                    if ( t[i][j] >= centobj )
                    {
                        x[i][j].setUB ( 0.0 ); // Use the bounds so that CPLEX can preprocess the variables away
                        t[i][j] = INT_MAX;
                    }
                }
            }

            // Calculate the computation time and push into the Times vector
            single_time = duration_cast<duration<double>> ( End-Start );
            Times.push_back ( static_cast<double> ( single_time.count() ) );
            Start = CPUclock::now();
        }

        // Calculate the overall time consumption and assign to TotalTime variable
        OverAllEnd = CPUclock::now();
        overall_time = duration_cast<duration<double>> ( OverAllEnd-OverAllStart );
        TotalTime = static_cast<double> ( overall_time.count() );

        // Loop the WeakPoints array and calculate the number of weakly dominated points. Also fill in the Frontier vector
        for ( size_t w=0; w<WeakPoints.size()-1; ++w )
        {
            if ( WeakPoints[w].first == WeakPoints[w+1].first ) ++NumOfWP;
            else Frontier.push_back ( WeakPoints[w] );
        }

        // Calculate the number of Pareto optimal solutions found
        NumOfPP = WeakPoints.size() - NumOfWP;
        //Outputting information
        std::ofstream output;

        if ( !output ) throw std::runtime_error ( "Coudl not create the ifstream.\n" );

        output.open ( outputfile );
        std::string error = "Could not open the output file " + outputfile + "\n";

        if ( !output ) throw std::runtime_error ( error.c_str() );

        output << "Frontier size:\t"    << NumOfPP << std::endl;
        output << "Weakly effici:\t"    << NumOfWP << std::endl;
        output << "Total time:\t"       << TotalTime << std::endl;
        output << "point\t time\n";

        for ( size_t t=0; t<Times.size(); ++t )
        {
            output << t << "\t" << Times[t] << std::endl;
        }

        output << "================== Frontier ==================\n";

        for ( size_t w=0; w<WeakPoints.size(); ++w ) output << WeakPoints[w].first << "\t" << WeakPoints[w].second << std::endl;

        output.close();
        output.open ( SummaryFile, std::ofstream::out | std::ofstream::app );

        if ( !output ) throw std::runtime_error ( "Could not open the summary file.\n" );

        output <<  InstanceNumber << "_" << CostMethod << "\t" << NumOfPP << "\t" << NumOfWP << "\t" << TotalTime << "\t"<< ( WeakPoints.back().first - WeakPoints[0].first ) <<
               "\t" << ( WeakPoints[0].second - WeakPoints.back().second ) << std::endl;
        output.close();
    }
    catch ( std::exception &e )
    {
        std::cerr << "Exception in the run function in PureCplex: " << e.what() << std::endl;
        exit ( EXIT_FAILURE );
    }
    catch ( IloException &ie )
    {
        std::cerr << "IloException in the run function in PureCplex: " << ie.getMessage() << std::endl;
        exit ( EXIT_FAILURE );
    }
}

//========================================================================================================//
int BOCBLPsolver::EvaluateCenter()
{
    int CentObj = 0, minT;

    for ( int j =0; j<m; ++j )
    {
        minT = INT_MAX;

        for ( int i=0; i<n; ++i ) if ( ceil ( cplex.getValue ( x[i][j] ) ) >0.5 && minT>t[i][j] ) minT = t[i][j];

        if ( minT>CentObj ) CentObj = minT;
    }

    return CentObj;
}

//========================================================================================================//
void BOCBLPsolver::BuildModel()
{
    try
    {
        model = IloModel ( env );
        cplex = IloCplex ( model );
        y = IloNumVarArray ( env, n, 0, 1, ILOBOOL );
        x = IloVarMatrix ( env,n );

        if ( ProblemType<=2 ) // The problem is either Pmedian, UFLP or CFLP and it will make due with continuous x-variables
        {
            for ( int i=0; i<n; ++i ) x[i] = IloNumVarArray ( env, m, 0, 1, ILOFLOAT );
        }
        else   // The problem is a SSCFLP and the x-variables need to be binary
        {
            for ( int i=0; i<n; ++i ) x[i] = IloNumVarArray ( env, m, 0, 1, ILOBOOL );
        }

        IloExpr expr ( env );

        for ( int i=0; i<n; ++i ) // Building the objective
        {
            if ( ProblemType>=1 ) expr+=f[i]*y[i]; // The problem is a UFLP, CFLP or SSCFLP and needs fixed costs

            for ( int j=0; j<m; ++j ) expr+=c[i][j]*x[i][j]; // All objectives need assignment costs
        }

        OBJ = IloMinimize ( env,expr );
        model.add ( OBJ );
        expr.clear();

        for ( int j=0; j<m; ++j ) // all models needs assignment constraints
        {
            for ( int i=0; i<n; ++i ) expr+=x[i][j];

            model.add ( expr == 1 );
            expr.clear();
        }

        if ( 0 == ProblemType || 1 == ProblemType ) // This is a Pmedian or an uflp problem and we need the variable upper bounds
        {
            for ( int i=0; i<n; ++i )
            {
                if ( !fToZero ) for ( int j=0; j<m; ++j ) model.add ( x[i][j]-y[i]<=0 );

                expr+=y[i];
            }

            if ( 0 == ProblemType ) model.add ( expr == p ); // The problem is a Pmedian problem and we need to specify that the p facilities should be open

            expr.clear();
        }
        else   // The problem is a cflp or sscflp. Anyway, they are identical from hereon (the single sourcing has been taken care of above)
        {
            IloExpr expr2 ( env );

            for ( int i=0; i<n; ++i ) // For each facility add a capacity constraint
            {
                for ( int j=0; j<m; ++j )
                {
                    expr+=d[j]*x[i][j];
                    model.add ( x[i][j]-y[i]<=0 );
                }

                if ( fToZero ) model.add ( expr <= s[i] );
                else model.add ( expr <= s[i]*y[i] );

                expr.clear();
                expr2+=s[i]*y[i];
            }

            //model.add(expr2>= TD);
            expr2.end();
        }

        for ( int i=0; i<n; ++i ) if ( f[i] <= 0.1 && ( ProblemType > 0 ) ) for ( int i=0; i<n; ++i ) y[i].setLB ( 1 ); // if the f[i] = 0 and it is a fixed charge FLP, set y[i] = 1

        expr.end();
        cplex.setOut ( env.getNullStream() );       // Tell cplex to be quiet
        cplex.setWarning ( env.getNullStream() );   // Tell cplex to be quiet
        cplex.setParam ( IloCplex::EpGap,0 );       // Set the allowed percentage gap to zero
        cplex.setParam ( IloCplex::EpAGap,0 );      // Set the allowed absolute gap to zero
        cplex.setParam ( IloCplex::MIPSearch,1 );   // Set the MIPsearch to 1. It seems to work better than default on location problems
        cplex.setParam ( IloCplex::ParallelMode,1 ); // Set the Parallelmode to deterministic
    }
    catch ( std::exception &e )
    {
        std::cerr << "Exception in the BuildModel function in PureCplex: " << e.what ( ) << std::endl;
        exit ( EXIT_FAILURE );
    }
    catch ( IloException &ie )
    {
        std::cerr << "IloException in the BuildModel function in PureCplex : " << ie.getMessage ( ) << std::endl;
        exit ( EXIT_FAILURE );
    }
}
