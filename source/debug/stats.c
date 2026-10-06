#include "common/general.h"

void initStats(stats_struct* s)
{
	s->total=0;
	s->min=1<<30;
	s->max=0;
	s->count=0;
}

void addValue(stats_struct* s, u32 value)
{
	s->total+=value;
	if(value>s->max)s->max=value;
	if(value<s->min)s->min=value;
	s->count++;
}

void writeStats(stats_struct* s, char* name, char* filename)
{
}
