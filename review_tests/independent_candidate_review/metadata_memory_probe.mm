#define main candidate_metrics_original_main
#include "game_lab/original_building_candidate_metrics.mm"
#undef main
#include <sys/resource.h>
int main(int argc,char**argv){int result=candidate_metrics_original_main(argc,argv);struct rusage u{};getrusage(RUSAGE_SELF,&u);std::cout<<"{\"result\":"<<result<<",\"peak_rss\":"<<u.ru_maxrss<<"}\n";return result;}
