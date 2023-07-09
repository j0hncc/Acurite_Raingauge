// ver 1.44  malloc version
class Rolling {
public:
	const char* label;
	long begintime, duration;  // in minutes; signed 2147483647 = 4083 years
	int *data;
	int siz;  // indices into data[]
	int curr; // "

	Rolling(int sizz, long durr,const char* lbl): label(lbl),begintime(0), duration(durr), siz(sizz){
            data = (int *) malloc(siz * sizeof(int));
            for (int i=0; i<siz; i++) data[i]=0;
        }
        
        ~Rolling(){
            free(data);
            data=NULL;
        }

	void rain(long clock, int amt){
		while (clock > begintime + duration){   // should be clock - begintime > duration ?
			advance();
			data[curr]=0;
			begintime += duration;
		}
		addAmount( amt);
	}

	int getAmount(int periodsBack){
		return data[ indexOf( periodsBack) ];
	}

        // "private"
	int indexOf(int periodsBack){
		if (periodsBack<0) periodsBack=-periodsBack;  // abs()
		int c=curr+siz-periodsBack;
		c = c % siz;
		return c;
	}

	// "restricted" to loading data
	void setHistory(int periodsBack, int newAmt){
		// UNTESTED 6/10/13
		if (periodsBack >= siz) return;  // too old, ignore
		long minutesBack= periodsBack * duration;
		if ( -minutesBack < begintime ) begintime = -minutesBack;
		data[ indexOf( periodsBack) ] = newAmt;
	}


	/*
	void report(){
		int i=0;
		cout << begintime  << "  ";
		for (i=0;i<siz; i++)
			cout << getAmount(i) << " ";
		cout << endl;
	}
	*/
private:
	void advance(){
		if (++curr >= siz) {
			curr=0;
		}
	}

	void setAmount(int amt){
		data[curr]= amt;
	}
	void addAmount(int amt){
		data[curr] += amt;
	}
	int previous(int curr){
		if (curr == 0) return siz-1;
		else return curr-1;
	}
};


