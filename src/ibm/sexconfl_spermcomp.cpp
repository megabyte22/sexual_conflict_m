// sexual conflict with multiple offense and resistance traits
// 
//
//     Copyright (C) 2010 Bram Kuijper
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#define NDEBUG
//#define DISTRIBUTION
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include <cassert>

#include "random.h"
#include "bram.h"

using namespace std;

const int N = 5000;
const int N_mate_sample = 10;
const int clutch_size = 10;
const int nTraits = 1;
double a = 1.0; // strength of selection against non-optimal # matings
double cs = 0.5; // cost of sensitivity
double ct = 0.5; // cost of     
double co = 0.2; // cost of trait
double const thr_opt = 2;
double const sen_opt = 3;
double const off_opt = 2;
double const init_sen = sen_opt; // traits start at their optima
double const init_off = off_opt; 
double const init_thr = thr_opt; 
double const powthr = 2;
double const powsen = 2;
double const powoff = 2;
double mu_off 	  = 0.05;            // mutation rate
double mu_thr 	  = 0.05;            // mutation rate
double mu_sen     = 0.05;            // mutation rate
double r = 0.1;
double sdmu_off         = 0.4;			 // standard deviation mutation size
double sdmu_thr         = 0.4;			 // standard deviation mutation size
double sdmu_sen         = 0.4;			 // standard deviation mutation size
const double NumGen = 50000;
double sperm_threshold = 0.0;
const int skip = 10;
int popsize = 0;
double theta_psi = 1;
double meanvarsperm = 0;

bool do_stats = 0;

int generation = 0;
double mean_matings_female = 0;
double var_matings_female = 0;
double mean_matings_male = 0;
double var_matings_male = 0;
int Nfemales = N / 2, Nmales = N / 2;
unsigned seed = 0;
int msurvivors = 0;
int fsurvivors = 0;
int unmated_f = 0;
int spermless_f = 0;

int father_eggs[N];
int mother_eggs[N];
int partners[N][N_mate_sample];

Stats stat_start_off[nTraits], stat_start_phen_off[nTraits], stat_ns_off[nTraits];
Stats stat_start_thr[nTraits], stat_start_phen_thr[nTraits], stat_ns_thr[nTraits];
Stats stat_start_sen[nTraits], stat_start_phen_sen[nTraits], stat_ns_sen[nTraits];
JointStats jstat_start_offsen[nTraits], jstat_ns_offsen[nTraits], jstat_ss_offsen[nTraits];
JointStats jstat_start_offthr[nTraits], jstat_ns_offthr[nTraits], jstat_ss_offthr[nTraits];
JointStats jstat_start_thrsen[nTraits], jstat_ns_thrsen[nTraits], jstat_ss_thrsen[nTraits];

// the individual struct
struct Individual
{
	double off[nTraits][2]; // male offense trait
	double thr[nTraits][2]; // female threshold
	double sen[nTraits][2]; // female sensitivity

    double ejaculate;
    int npartners;
    int fecundity;

    double e_off[nTraits];
    double e_thr[nTraits];
    double e_sen[nTraits];
};

typedef Individual Population[N];
Population Females, Males, FemaleSurvivors, MaleSurvivors;
typedef Individual Kids[N*clutch_size];
Kids NewPop;

string filename("sim_sexualconflict_sc");
string filename_new(create_filename(filename));
ofstream DataFile(filename_new.c_str());  // output file 

#ifdef DISTRIBUTION
string filename_new2(create_filename("sim_sexualconflict_sc"));
ofstream distfile(filename_new2.c_str());
#endif //DISTRIBUTION

void initArguments(int argc, char *argv[])
{
	a = atof(argv[1]);
	co = atof(argv[2]);
	ct = atof(argv[3]);
	cs = atof(argv[4]);
	mu_off = atof(argv[5]);
	mu_thr = atof(argv[6]);
	mu_sen = atof(argv[7]);
	sdmu_off = atof(argv[8]);
	sdmu_thr = atof(argv[9]);
	sdmu_sen = atof(argv[10]);
    theta_psi = atof(argv[11]);
    r = atof(argv[12]);
}

void MutateOff(double &G)
{
	G += Uniform()<mu_off ? Normal(0,sdmu_off) : 0;
}

void MutateThr(double &G)
{
	G+= Uniform()<mu_thr ? Normal(0,sdmu_thr) : 0; 
}

void MutateSen(double &G)
{
	G+= Uniform()<mu_sen ? Normal(0,sdmu_sen) : 0; 
}

void WriteParameters()
{
	DataFile << endl
		<< endl
		<< "type:;" << "sexual_conflict_sperm_comp" << ";" << endl
		<< "popsize_init:;" << N << ";" << endl
		<< "n_mate_sample:;" << N_mate_sample << ";"<< endl
		<< "nTraits:;" << nTraits << ";"<< endl
		<< "a:;" <<  a << ";"<< endl
		<< "coff:;" <<  co << ";"<< endl
		<< "cthr:;" <<  ct << ";"<< endl
		<< "csen:;" <<  cs << ";"<< endl
		<< "thr_opt:;" <<  thr_opt << ";"<< endl
		<< "sen_opt:;" <<  sen_opt << ";"<< endl
		<< "off_opt:;" <<  off_opt << ";"<< endl
		<< "theta_psi:;" << theta_psi << ";"<< endl
		<< "mu_off:;" <<  mu_off << ";"<< endl
		<< "mu_thr:;" <<  mu_thr << ";"<< endl
		<< "mu_sen:;" <<  mu_sen << ";"<< endl
		<< "mu_std_off:;" <<  sdmu_off << ";"<< endl
		<< "mu_std_thr:;" <<  sdmu_thr << ";"<< endl
		<< "mu_std_sen:;" <<  sdmu_sen << ";"<< endl
		<< "seed:;" << seed << ";"<< endl;
}

// initialize all the phenotypes
void Init()
{
	seed = get_nanoseconds();

	SetSeed(seed);    
    
    for (int i = 0; i < N/2; ++i)
    {
        for (int j = 0; j < nTraits; ++j)
        {
            Females[i].off[j][0] = init_off;
            Females[i].off[j][1] = init_off;
            Females[i].sen[j][0] = init_sen;
            Females[i].sen[j][1] = init_sen;
            Females[i].thr[j][0] = init_thr;
            Females[i].thr[j][1] = init_thr;
            
            Males[i].off[j][0] = init_off;
            Males[i].off[j][1] = init_off;
            Males[i].sen[j][0] = init_sen;
            Males[i].sen[j][1] = init_sen;
            Males[i].thr[j][0] = init_thr;
            Males[i].thr[j][1] = init_thr;
            
            Females[i].e_thr[j] =  init_thr;
            Females[i].e_sen[j] =  init_sen;
            Males[i].e_off[j] =  init_off;
        }

        Females[i].ejaculate= 0;
        Males[i].ejaculate= 0;
    }
    
    Nmales = N/2;
    Nfemales = N/2; 
    popsize = N;
}

void Create_Kid(int mother, int father, Individual &kid)
{
	assert(mother >= 0 && mother < fsurvivors);
	assert(father >= 0 && father < msurvivors);

    for (int i = 0; i < nTraits; ++i)
    {
        kid.off[i][0] = FemaleSurvivors[mother].off[i][RandomNumber(2)];
        MutateOff(kid.off[i][0]);
        kid.off[i][1] = MaleSurvivors[father].off[i][RandomNumber(2)];
        MutateOff(kid.off[i][1]);

        kid.thr[i][0] = FemaleSurvivors[mother].thr[i][RandomNumber(2)];
        MutateThr(kid.thr[i][0]);
        kid.thr[i][1] = MaleSurvivors[father].thr[i][RandomNumber(2)];
        MutateThr(kid.thr[i][1]);

        kid.sen[i][0] = FemaleSurvivors[mother].sen[i][RandomNumber(2)];
        MutateSen(kid.sen[i][0]);
        kid.sen[i][1] = MaleSurvivors[father].sen[i][RandomNumber(2)];
        MutateSen(kid.sen[i][1]);
    }
}


// threshold and sensitivity affect survival
// psi affects fecundity
// we vary this later
void Survive()
{
    fsurvivors = 0;
    double w = 0;

    if (do_stats)
    {
        for (int i = 0; i < nTraits; ++i)
        {
            stat_reset(stat_ns_off[i]);
            stat_reset(stat_ns_thr[i]);
            stat_reset(stat_ns_sen[i]);
        }
    }
    
	for (int i = 0; i < Nfemales; ++i)
	{
        double thr = 0;
        double sen = 0;

        for (int j = 0; j < nTraits; ++j)
        {
            thr += pow(Females[i].e_thr[j] - thr_opt, powthr);
            sen += pow(Females[i].e_sen[j] - sen_opt, powsen);
        }

        // we use the same scaling of fitness as Day et al use
        // but we use exponents here:
        w = exp(-ct * thr - cs * sen);

        if (Uniform() < w)
        {
            if (do_stats)
            {
                for (int j = 0; j < nTraits; ++j)
                {
                    stat_addval(stat_ns_thr[j], Females[i].e_thr[j]);
                    stat_addval(stat_ns_sen[j], Females[i].e_sen[j]);
                }
            }
            FemaleSurvivors[fsurvivors++] = Females[i];
        }
	}

    msurvivors = 0;

	// now get the male trait values into a viability measure
	for (int i = 0; i < Nmales; ++i)
	{
        double off = 0;

        for (int j = 0; j < nTraits; ++j)
        {
            off += pow(Males[i].e_off[j] - off_opt, powoff);
        }

		Males[i].ejaculate = exp(-co * off);
        
        MaleSurvivors[msurvivors++] = Males[i];
	}

    if (fsurvivors == 0 || msurvivors == 0)
    {
        WriteParameters();

        exit(1);
    }

    assert(fsurvivors > 0 && fsurvivors < popsize);
    assert(msurvivors > 0 && msurvivors < popsize);

}

void Choose(int &mother) 
{
	// check if there are enough other individuals to choose from
	// otherwise restrict the sample to the amount of individuals present
	int encounters = N_mate_sample > msurvivors ? msurvivors : N_mate_sample;

    assert(FemaleSurvivors[mother].npartners == 0);
    // encounter a number of males 
    // mate with them only if the threshold and sensitivity allow it
    // when they mate this will be costly
	for (int i = 0; i < encounters; ++i)
	{
		// get a random male survivor
		int random_mate = RandomNumber(msurvivors);
        
        double signal = 0;

        // go through the number of traits and calculate the mating rate
        for (int j = 0; j < nTraits; ++j)
        {
            signal += FemaleSurvivors[mother].e_sen[j] * (MaleSurvivors[random_mate].e_off[j] - FemaleSurvivors[mother].e_thr[j]);
        }

        double psi = 1 / (1 + ((1/theta_psi) - 1) * exp(-signal)); // no signal: psi = 0.5
    
        // will this male mate with this female?
        if (Uniform() < psi)
        {
            partners[mother][FemaleSurvivors[mother].npartners++] = random_mate;
            MaleSurvivors[random_mate].npartners++;

            assert(FemaleSurvivors[mother].npartners > 0 && FemaleSurvivors[mother].npartners <= encounters);
            assert(MaleSurvivors[random_mate].npartners > 0 && MaleSurvivors[random_mate].npartners <= fsurvivors);
        }
	}

    // the more the total number of matings diverges from the optimum, the lower
    // the fecundity
    double fecundity_part = (double) clutch_size * exp(-a * pow((double) FemaleSurvivors[mother].npartners / encounters - theta_psi, 2.0));

    int fecundity = floor(fecundity_part);

    double delta_round = fecundity_part - fecundity;

    if (Uniform() < delta_round)
    {
        ++fecundity;
    }

    FemaleSurvivors[mother].fecundity = fecundity;
} // end ChooseMates

// choose the mate
// this is a simple unilateral mate choice function
// later on we have to think about mutual mate choice
void NextGen()
{
    int offspring = 0;
    meanvarsperm = 0;
    unmated_f = 0;
    spermless_f = 0;

    int sum_partners_female = 0;
    int ss_partners_female = 0;
    
    // let the surviving females choose a mate
	for (int i = 0; i < fsurvivors; ++i)
	{
        mother_eggs[i] = 0;
        FemaleSurvivors[i].fecundity = 0;
		Choose(i);

        sum_partners_female += FemaleSurvivors[i].npartners;

        ss_partners_female += FemaleSurvivors[i].npartners * FemaleSurvivors[i].npartners;
	}

    mean_matings_female = (double) sum_partners_female / fsurvivors;
    var_matings_female = (double) ss_partners_female / fsurvivors - mean_matings_female * mean_matings_female;

    // calculate the variance in male partners
    if (do_stats)
    {
        int ss_partners_male = 0;
        int sum_partners_male = 0;

        for (int i = 0; i < msurvivors; ++i)
        {
            father_eggs[i] = 0;
        
            sum_partners_male += MaleSurvivors[i].npartners; 
            ss_partners_male += MaleSurvivors[i].npartners*MaleSurvivors[i].npartners;
        }
        
        mean_matings_male = (double) sum_partners_male / msurvivors;
        var_matings_male = (double) ss_partners_male / msurvivors - mean_matings_male * mean_matings_male;
    }
    
    double sperm_dist[N_mate_sample];

    // okay, females are mated, happily or not
    // now we create babies and allow sperm competition to have its way
    for (int i = 0; i < fsurvivors; ++i)
    {
        assert(FemaleSurvivors[i].npartners >= 0 && FemaleSurvivors[i].npartners <= msurvivors);
        assert(FemaleSurvivors[i].fecundity >= 0 && FemaleSurvivors[i].fecundity <= clutch_size);
        
        double sum_sperm = 0;
        double ss_sperm = 0;

        // alas, female has no eggs...
        if (FemaleSurvivors[i].fecundity < 1)
        { 
            continue;
        }

        int npartners = FemaleSurvivors[i].npartners;
        
        if (npartners < 1) 
        {
            ++unmated_f; // keep track of unmated females
            continue;
        }

        // loop through all a female's partners
        for (int j = 0; j < npartners; ++j)
        {
            int father = partners[i][j];

            assert(father >= 0 && father < msurvivors);

            // get male investment into gametes
            assert(MaleSurvivors[father].ejaculate >= 0 && MaleSurvivors[father].ejaculate <= 1);
            assert(MaleSurvivors[father].npartners > 0 && MaleSurvivors[father].npartners <= fsurvivors);

            // add this investment to a cumulative distribution of ejaculates
            // from which sires will be drawn
            double load = j == npartners - 1 ? 1.0 : r;

            double transfer = load * MaleSurvivors[father].ejaculate / MaleSurvivors[father].npartners;

            if (transfer < sperm_threshold)
            {
                transfer = 0.0;
            }

            sum_sperm += transfer;

            // make the sperm distribution inverse relative
            // meaning that the first value has relative amount 1 = s[1] / s[1]
            // the second value relative amount s[2] / (s[1] + s[2])
            // the third value relative amount s[3] / (s[1] + s[2] + s[3])
            // the last value is fully relative to the total sperm contributions
            sperm_dist[j] = transfer / sum_sperm;

            ss_sperm += transfer*transfer;
        }

        meanvarsperm += npartners > 0 && sum_sperm > 0 ? ss_sperm / npartners - sum_sperm * sum_sperm / (npartners * npartners) : 0;

        // sort out fathers only if females have received any sperm
        if (sum_sperm > 0)
        {
            int eggs_left = FemaleSurvivors[i].fecundity;

            // loop backwards through the partners
            // starting with the last guy which has a fully relative sperm contribution
            for (int j = FemaleSurvivors[i].npartners - 1; j >= 0; --j)
            {
                double n_eggs = sperm_dist[j] * eggs_left; 

                int n_eggs_i = floor(n_eggs);

                if (Uniform() < n_eggs - n_eggs_i)
                {
                    ++n_eggs_i;
                }

                eggs_left -= n_eggs_i;

                assert(eggs_left >= 0);

                int father = partners[i][j];

                assert(father >= 0 && father < msurvivors);

                for (int k = 0; k < n_eggs_i; ++k)
                {
                    Individual Kid;
                    Create_Kid(i, father, Kid);
                    NewPop[offspring++] = Kid;

                    if (do_stats)
                    {
                        ++father_eggs[father];
                        ++mother_eggs[i];
                    }
                }
            }
            assert(eggs_left == 0);
        }
        else
        {
            ++spermless_f;
        }

    } // end loop through fsurvivors

    meanvarsperm /= (double) fsurvivors;
    // population extinction, either due to lack of male fertility
    // or female fecundity? exit.
    if (offspring == 0)
    {
        WriteParameters();
        exit(1);
    }
            
    assert(offspring > 0 && offspring < popsize*clutch_size);

    int sons = 0;
    int daughters = 0;
    
    popsize = offspring < N ? offspring : N;

    for (int i = 0; i < popsize; ++i)
    {
        if (Uniform() < 0.5)
        {
            Males[sons] = NewPop[RandomNumber(offspring)];
   
            for (int j = 0; j < nTraits; ++j)
            {
                double off = 0.5 * ( Males[sons].off[j][0] + Males[sons].off[j][1]);
                Males[sons].e_off[j] = off; 
                Males[sons].npartners = 0;
            }
            ++sons;
        }
        else
        {
            Females[daughters] = NewPop[RandomNumber(offspring)];
            
            for (int j = 0; j < nTraits; ++j)
            {
                double thr = 0.5 * ( Females[daughters].thr[j][0] + Females[daughters].thr[j][1]);
                Females[daughters].e_thr[j] = thr;
                double sen = 0.5 * ( Females[daughters].sen[j][0] + Females[daughters].sen[j][1]);
                Females[daughters].e_sen[j] = sen;
                Females[daughters].npartners = 0;
            }
            ++daughters;
        }
    }

    Nmales = sons;
    Nfemales = daughters;
}

void WriteData()
{
	if (Nmales == 0 || Nfemales == 0)
	{
		WriteParameters();
		exit(1);
	}

    for (int i = 0; i < nTraits; ++i)
    {
        stat_reset(stat_start_phen_off[i]);
        stat_reset(stat_start_phen_thr[i]);
        stat_reset(stat_start_phen_sen[i]);
        stat_reset(stat_start_off[i]);
        stat_reset(stat_start_thr[i]);
        stat_reset(stat_start_sen[i]);
        jstat_reset(jstat_start_offsen[i]);
        jstat_reset(jstat_start_offthr[i]);
        jstat_reset(jstat_start_thrsen[i]);
    }
    
    int sumfrs = 0; 
    int summrs = 0; 
    int ssfrs = 0; 
    int ssmrs = 0; 

    double meanmrs, meanfrs, varfrs, varmrs;

	for (int i = 0; i < Nmales; ++i)
	{
        double off, sen, thr;

        for (int j = 0; j < nTraits; ++j)
        {
            off = 0.5 * ( Males[i].off[j][0] + Males[i].off[j][1]);
            sen = 0.5 * ( Males[i].sen[j][0] + Males[i].sen[j][1]);
            thr = 0.5 * ( Males[i].thr[j][0] + Males[i].thr[j][1]);

#ifdef DISTRIBUTION 
            distfile << generation << ";"
                    << setprecision(5) << off << ";"
                    << setprecision(5) << sen << ";"
                    << setprecision(5) << 0 << ";" 
                    << setprecision(5) << thr << ";" << endl;
#endif // DISTRIBUTION
            
            stat_addval(stat_start_off[j], off);
            stat_addval(stat_start_thr[j], thr);
            stat_addval(stat_start_sen[j], sen);
            stat_addval(stat_start_phen_off[j], Males[i].e_off[j]);
            jstat_addval(jstat_start_offsen[j], off, sen); 
            jstat_addval(jstat_start_offthr[j], off, thr); 
            jstat_addval(jstat_start_thrsen[j], sen, thr); 
        }
        
        if (i < msurvivors)
        {
            summrs += father_eggs[i];
            ssmrs += father_eggs[i] * father_eggs[i];
        }
	}

	for (int i = 0; i < Nfemales; ++i)
	{
        double off, sen, thr;
        for (int j = 0; j < nTraits; ++j)
        {
            off = 0.5 * ( Females[i].off[j][0] + Females[i].off[j][1]);
            sen = 0.5 * ( Females[i].sen[j][0] + Females[i].sen[j][1]);
            thr = 0.5 * ( Females[i].thr[j][0] + Females[i].thr[j][1]);


#ifdef DISTRIBUTION 
            distfile << generation << ";"
                    << setprecision(5) << off << ";"
                    << setprecision(5) << sen << ";"
                    << setprecision(5) << 0 << ";" 
                    << setprecision(5) << thr << ";" << endl;
#endif // DISTRIBUTION
            stat_addval(stat_start_phen_thr[j], Females[i].e_thr[j]);
            stat_addval(stat_start_phen_sen[j], Females[i].e_sen[j]);
            stat_addval(stat_start_off[j], off);
            stat_addval(stat_start_thr[j], thr);
            stat_addval(stat_start_sen[j], sen);
            jstat_addval(jstat_start_offsen[j], off, sen); 
            jstat_addval(jstat_start_offthr[j], off, thr); 
            jstat_addval(jstat_start_thrsen[j], sen, thr); 
        }
            
        if (i < fsurvivors)
        {
            sumfrs += mother_eggs[i];
            ssfrs += mother_eggs[i] * mother_eggs[i];
        }

	}

    for (int i = 0; i < nTraits; ++i)
    {
        stat_finalize(stat_start_phen_thr[i]);
        stat_finalize(stat_start_thr[i]);
        stat_finalize(stat_ns_thr[i]);

        stat_finalize(stat_start_phen_sen[i]);
        stat_finalize(stat_start_sen[i]);
        stat_finalize(stat_ns_sen[i]);
        
        stat_finalize(stat_start_phen_off[i]);
        stat_finalize(stat_start_off[i]);
        stat_finalize(stat_ns_off[i]);

        jstat_finalize(jstat_start_offsen[i], stat_start_off[i].mean, stat_start_sen[i].mean);
        jstat_finalize(jstat_start_offthr[i], stat_start_off[i].mean, stat_start_thr[i].mean);
        jstat_finalize(jstat_start_thrsen[i], stat_start_sen[i].mean, stat_start_thr[i].mean);
    }

    double sum_sexes = Nmales + Nfemales;
    
    meanfrs = (double) sumfrs / Nfemales;
    meanmrs = (double) summrs / Nmales;
    varfrs = (double) ssfrs / Nfemales - meanfrs * meanfrs;
    varmrs = (double) ssmrs / Nmales - meanmrs * meanmrs;


	DataFile << generation;

    for (int i = 0; i < nTraits; ++i)
    {
		DataFile << ";" << setprecision(5) << stat_start_off[i].mean
		<< ";" << setprecision(5) << stat_start_thr[i].mean
		<< ";" << setprecision(5) << stat_start_sen[i].mean
		
		<< ";" << setprecision(5) << stat_start_off[i].mean_ci
		<< ";" << setprecision(5) << stat_start_thr[i].mean_ci
		<< ";" << setprecision(5) << stat_start_sen[i].mean_ci

		<< ";" << setprecision(5) << stat_start_phen_off[i].mean
		<< ";" << setprecision(5) << stat_start_phen_thr[i].mean
		<< ";" << setprecision(5) << stat_start_phen_sen[i].mean
		
		<< ";" << setprecision(5) << stat_ns_off[i].mean
		<< ";" << setprecision(5) << stat_ns_thr[i].mean
		<< ";" << setprecision(5) << stat_ns_sen[i].mean
		
        << ";" << setprecision(5) << stat_ns_off[i].mean_ci
		<< ";" << setprecision(5) << stat_ns_thr[i].mean_ci
		<< ";" << setprecision(5) << stat_ns_sen[i].mean_ci

		<< ";" << setprecision(5) << stat_start_off[i].var
		<< ";" << setprecision(5) << stat_start_thr[i].var
		<< ";" << setprecision(5) << stat_start_sen[i].var
		
        << ";" << setprecision(5) << stat_start_phen_off[i].var
		<< ";" << setprecision(5) << stat_start_phen_thr[i].var
		<< ";" << setprecision(5) << stat_start_phen_sen[i].var

		<< ";" << setprecision(5) << stat_ns_off[i].var
		<< ";" << setprecision(5) << stat_ns_thr[i].var
		<< ";" << setprecision(5) << stat_ns_sen[i].var

		<< ";" << setprecision(5) << jstat_start_offsen[i].cov 
		<< ";" << setprecision(5) << (jstat_start_offsen[i].cov == 0 ? 0 : jstat_start_offsen[i].cov / (sqrt(stat_start_off[i].var) * sqrt(stat_start_sen[i].var)))

		<< ";" << setprecision(5) << jstat_start_offthr[i].cov 
		<< ";" << setprecision(5) << (jstat_start_offthr[i].cov == 0 ? 0 : jstat_start_offthr[i].cov / (sqrt(stat_start_off[i].var) * sqrt(stat_start_thr[i].var)))
		
        << ";" << setprecision(5) << jstat_start_thrsen[i].cov 
		<< ";" << setprecision(5) << (jstat_start_thrsen[i].cov == 0 ? 0 : jstat_start_thrsen[i].cov / (sqrt(stat_start_sen[i].var) * sqrt(stat_start_thr[i].var)));
    }

        DataFile << ";" << msurvivors
        << ";" << fsurvivors
        << ";" << setprecision(5) <<   meanvarsperm 
        << ";" << setprecision(5) <<  mean_matings_male
        << ";" << setprecision(5) <<  var_matings_male
        << ";" << setprecision(5) <<  mean_matings_female
        << ";" << setprecision(5) <<  var_matings_female
        << ";" << setprecision(5) <<  unmated_f
        << ";" << setprecision(5) <<  spermless_f
        << ";" << setprecision(5) << meanfrs 
        << ";" << setprecision(5) << meanmrs 
        << ";" << setprecision(5) << varfrs 
        << ";" << setprecision(5) << varmrs 
		<< ";" << sum_sexes
        << ";" << endl;
}

void WriteDataHeaders()
{
	DataFile << "generation";

    for (int i = 1; i <= nTraits; ++i)
    {
		DataFile << ";mean_off" << i 
		<< ";mean_thr" << i
		<< ";mean_sen" << i

        << ";ci_off"<< i
        << ";ci_thr"<< i
        << ";ci_sen"<< i

		<< ";mean_phen_off" << i
		<< ";mean_phen_thr" << i
		<< ";mean_phen_sen" << i

		<< ";mean_ns_off" << i
		<< ";mean_ns_thr" << i
		<< ";mean_ns_sen"<< i
		
        << ";mean_ns_off_ci" << i
		<< ";mean_ns_thr_ci" << i
		<< ";mean_ns_sen_ci"<< i

		<< ";var_off" << i
		<< ";var_thr" << i
		<< ";var_sen" << i

		<< ";var_phen_off" << i
		<< ";var_phen_thr" << i
		<< ";var_phen_sen" << i

		<< ";var_ns_off" << i
		<< ";var_ns_thr" << i
		<< ";var_ns_sen" << i

		<< ";cov_offsen" << i
		<< ";corr_offsen" << i
		
        << ";cov_offthr" << i
		<< ";corr_offthr" << i

        << ";cov_thrsen" << i
		<< ";corr_thrsen" << i;
    }

    DataFile << ";surviving_males"
        << ";surviving_females"
        << ";var_sperm_per_female"
        << ";mean_matings_per_male"
        << ";var_matings_per_male"
        << ";mean_matings_per_female"
        << ";var_matings_per_female"
        << ";unmated_f"
        << ";spermless_f"
        << ";meanfrs"
        << ";meanmrs"
        << ";varfrs"
        << ";varmrs"
		<< ";N;"
		<< endl;
}

int main(int argc, char ** argv)
{
	initArguments(argc, argv);
	WriteDataHeaders();
	Init();

	for (generation = 0; generation <= NumGen; ++generation)
	{
		do_stats = generation % skip == 0;

		Survive();
		
        NextGen();
        
        if (do_stats)
		{
			WriteData();
		}
	}

	WriteParameters();
}
