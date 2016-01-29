#include "rdDat.h"

rdDat::rdDat ( std::string Filename, int ProblemType )
{
    try
    {
        TheProblemType = ProblemType;
        d = nullptr;
        s = nullptr;
        f = nullptr;
        c = nullptr;
        t = nullptr;

        if ( 0 == ProblemType )
        {
            rdPmed ( Filename );
        }
        else if ( 1 == ProblemType )
        {
            rdSSCFLP ( Filename );

            for ( int i=0; i<n; ++i ) s[i] = m;

            for ( int j=0; j<m; ++j ) d[j] = 1;
        }
        else if ( 2 == ProblemType )
        {
            rdSSCFLP ( Filename );
        }
        else if ( 3 == ProblemType )
        {
            rdSSCFLP ( Filename );
        }
        else
        {
            throw std::runtime_error ( "The specified problem type was not supported. Problem type has to be an integer. 0=Pmedian, 1=UFLP, 2=CFLP, 3=SSCFLP. Terminating!\n" );
        }
    }
    catch ( std::exception & e )
    {
        std::cerr << "Exception in CFLP constructor: " << e.what() << std::endl;
        exit ( 1 );
    }
}
//======================================================================================================
void rdDat::rdPmed ( std::string DataFile )
{
    try
    {
        double anyNum;
        std::ifstream InFile;
        std::stringstream err;

        if ( !InFile ) throw std::runtime_error ( "Cannot create the ifstream!\n" );

        InFile.open ( DataFile );

        if ( !InFile )
        {
            err << "Cannot open the input file:\t" << DataFile << "\n";
            throw std::runtime_error ( err.str() );
        }

        /* Format:
         * n = number of nodes in the graph. We set n=m just to be sure.
         * p = The number of open facilites in an optimal solution.
         * c_ij = cost matrix
         * t_ij = travel time matrix
         */
        InFile>>anyNum;

        if ( !InFile ) throw std::runtime_error ( "Could not retrieve the number of facilities.\n" );

        if ( anyNum < 1 ) throw std::runtime_error ( "A non-positive number of vertices has been specified. That is not meaningfull!\n" );

        n = anyNum;
        InFile>>anyNum;

        if ( !InFile ) throw std::runtime_error ( "Could not retrieve the number of facilities.\n" );

        if ( anyNum < 1 ) throw std::runtime_error ( "A non-positive number of vertices has been specified. That is not meaningfull!\n" );

        m = anyNum;
        InFile >> anyNum;

        if ( !InFile ) throw std::runtime_error ( "Could not retrieve the number of open facilities, p.\n" );

        p = anyNum;
        c = new double*[n];
        t = new int*[n];

        for ( int i=0; i<n; ++i ) //  Reading the cost matrix
        {
            c[i] = new double[n];

            for ( int j=0; j<n; ++j )
            {
                InFile >> anyNum;

                if ( !InFile )
                {
                    err << "Could not read the cost matrix entry " << i << "," << j << std::endl;
                    throw std::runtime_error ( err.str() );
                }

                c[i][j] = anyNum;
            }
        }

        for ( int i=0; i<n; ++i )
        {
            t[i] = new int[n];

            for ( int j=0; j<n; ++j )
            {
                InFile >> anyNum;

                if ( !InFile )
                {
                    err << "Could not read the travel time matrix entry " << i << "," << j << std::endl;
                    throw std::runtime_error ( err.str() );
                }

                t[i][j] = anyNum;
            }
        }
    }
    catch ( std::exception &e )
    {
        std::cerr << "Exception in rdPmed in the rdDat class: " << e.what() << std::endl;
    }
}

//======================================================================================================
void rdDat::rdSSCFLP ( std::string DataFile )
{
    try
    {
        double anyInt;
        std::ifstream InFile;
        std::stringstream err;

        if ( !InFile ) throw std::runtime_error ( "Cannot create the ifstream!\n" );

        InFile.open ( DataFile );

        if ( !InFile )
        {
            err << "Cannot open the input file:\t" << DataFile << "\n";
            throw std::runtime_error ( err.str() );
        }

        // Format:
        // n (number of facilities)
        // m (number of customers)
        // s_1 f_1
        // s_2 f_2
        // ...
        // s_n f_n
        //d_1 d_2 d_3 ... d_m
        // c_ij
        InFile>>anyInt;

        if ( !InFile ) throw std::runtime_error ( "Could not retrieve the number of facilities.\n" );

        n = anyInt;
        InFile >> anyInt;

        if ( !InFile ) throw std::runtime_error ( "Could not retrieve the number of customers.\n" );

        m = anyInt;
        c = new double*[n];
        t = new int*[n];
        d = new int[m];
        s = new int[n];
        f = new int[n];

        for ( int i=0; i<n; ++i )
        {
            InFile >> anyInt;

            if ( !InFile )
            {
                err << "Cannot read capacity " << i << ". Terminating!\n";
                throw std::runtime_error ( err.str() );
            }

            s[i] = anyInt;
            InFile >> anyInt;

            if ( !InFile )
            {
                err << "Cannot read fixed cost " << i << ". Terminating!\n";
                throw std::runtime_error ( err.str() );
            }

            f[i] = anyInt;
        }

        for ( int j=0; j<m; ++j )
        {
            InFile >> anyInt;

            if ( !InFile )
            {
                err << "Cannot read demand " << j << ". Terminating!\n";
                throw std::runtime_error ( err.str() );
            }

            d[j] = anyInt;
        }

        for ( int i=0; i<n; ++i )
        {
            c[i] = new double[m];

            for ( int j=0; j<m; ++j )
            {
                InFile >> anyInt;

                if ( !InFile )
                {
                    err << "Cannot read assignement cost " << i << " " << j << ". Terminating!\n";
                    throw std::runtime_error ( err.str() );
                }

                c[i][j]=anyInt;
            }
        }

        for ( int i=0; i<n; ++i )
        {
            t[i] = new int[m];

            for ( int j=0; j<m; ++j )
            {
                InFile>>anyInt;

                if ( !InFile )
                {
                    err << "Cannot read traportation time " << i << " " << j << ". Terminating!\n";
                    throw std::runtime_error ( err.str() );
                }

                t[i][j] = anyInt;
            }
        }
    }
    catch ( std::exception &e )
    {
        std::cerr << "Exception in rdSSCFLP in rdDat class: " << e.what() << std::endl;
    }
}


rdDat::~rdDat()
{
    if ( d != nullptr ) delete[] d;

    if ( s != nullptr ) delete[] s;

    if ( f != nullptr ) delete[] f;

    if ( c != nullptr )
    {
        if ( c[0] != nullptr ) for ( int i=0; i<n; ++i ) delete[] c[i];

        delete[] c;
    }

    if ( t != nullptr )
    {
        if ( t[0] != nullptr ) for ( int i=0; i<n; ++i ) delete[] t[i];

        delete[] t;
    }
}
