/* ***************************************************************************************
 * Stats 
 * v 1.24 6/09/13 
 * v 1.23 3/22/13 remove console/cout, add weekly/monthly, report uptime handling
 * v 1.20 remove advance
 * v 1.19  add advance()
 * ***************************************************************************************/
#if defined(ARDUINO)
typedef Stream OSTREAM;
// Stream & console( Serial);
#else  // std c++
typedef ostream OSTREAM;
ostream& console(cout);
#endif
class Stats {
public:
        static long upminutes;  // for reporting
        // static Rolling rs[];  // DO THIS!!
	static Rolling L24;
	static Rolling L7;
	static Rolling L1; 
	static Rolling L4; 
	static Rolling L12; 
	static void report(Rolling & r, OSTREAM & s);
	static void record(long mins, int hundredths);
} stats;

Rolling Stats::L1 (60, 1, "Last 60 min");  	// last 60 minutes of minutely (1) data
Rolling Stats::L24(24,60, "Last 24 hr");	// last 24hrs of hourly (60 min) data
Rolling Stats::L7(7,60*24, "Last 7 day");	// last 7days of daily (60*24 min) data
Rolling Stats::L4(4,60*24*7, "Last 4 wk");	// last 4weeks of weekly (60*24*7) data
Rolling Stats::L12(12,60L*24*(365.242/12), "Last 12 mo");	// last 12 months of monthly (60*24*365.242/12) data
long Stats::upminutes = 0;

// WORKS!! Rolling Stats::rs[] = { Rolling( 60, 1, "60-1"), Rolling( 24, 60, "24-60") };

void Stats::report(Rolling & r, OSTREAM & s)
{
        char zero='.', na=' ';
	int i=0; 
	int tot=0;
        boolean incomplete = false;
	s << r.label << "  " ;
	// s << br << r.begintime  << "  ";
	for (i=0;i<r.siz; i++) 
        {
            s << ' ';
            long oldesttime = r.duration * i;
            // Serial << upminutes << " " << r.begintime  << " " << r.duration  << " " << i << endl;
            if ( upminutes >= oldesttime )
            {
                if ( r.getAmount(i) > 0 ) 
                   s <<  r.getAmount(i) ;
                else 
                   s << zero ;
                tot += r.getAmount(i);
            } 
            else
            {
                s << na;
                incomplete=true;
            }
	}
	s << " = " << tot << (incomplete? '.' : ' ') << br ;
}

void Stats::record(long mins, int hundredths)
{
	L1.rain(mins, hundredths);
	L24.rain( mins,hundredths);
	L7.rain( mins,hundredths);
	L4.rain( mins,hundredths);
	L12.rain( mins,hundredths);
}



