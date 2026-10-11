#include "app_launch.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <fstream>
#include <iostream>
using namespace game_lab;
namespace {int checks=0;void require(bool ok,const char* why){++checks;if(!ok){std::cerr<<why<<'\n';std::exit(1);}}}
int main(int argc,char** argv) {
    require(argc==2,"own fixture directory");const std::filesystem::path dir=argv[1];std::filesystem::create_directories(dir);
    const auto bundled=dir/"bundle empty data",explicitDir=dir/"explicit input";
    std::filesystem::create_directories(bundled);std::filesystem::create_directories(explicitDir);
    self_fixture::numbers("own-empty-launch-directories",{2});
    size_t choices=0;const auto choose=[&]{++choices;return explicitDir.string();};
    auto p=prepareLaunch({},"",choose,bundled.string());
    require(p.action==LaunchAction::Start&&p.bundledSelfAuthored&&p.root.configured()&&choices==0,"normal startup uses packaged empty directory without chooser");
    require(p.root.resolve("DATA/not-present.map").empty(),"normal root has no original resources");
    p=prepareLaunch({"--data-root",explicitDir.string()},"relative",choose,bundled.string());
    require(p.action==LaunchAction::Start&&!p.bundledSelfAuthored&&choices==0,"explicit argument preserves precedence");
    p=prepareLaunch({},explicitDir.string(),choose,bundled.string());
    require(p.action==LaunchAction::Start&&!p.bundledSelfAuthored&&choices==0,"explicit environment preserves precedence");
    for(const auto& bad:std::vector<std::string>{"relative",(dir/"absent").string()}) {
        p=prepareLaunch({},"",choose,bad);require(p.action==LaunchAction::RootError&&!p.root.configured()&&!p.bundledSelfAuthored&&choices==0,"bad bundled root fails without media fallback");
        p=prepareLaunch({"--data-root",bad},"",choose,bundled.string());require(p.action==LaunchAction::RootError&&choices==0,"bad explicit root cannot use bundled fallback");
        p=prepareLaunch({},bad,choose,bundled.string());require(p.action==LaunchAction::RootError&&choices==0,"bad environment cannot use bundled fallback");
    }
    for(auto args:std::vector<std::vector<std::string>>{{"--data-root"},{"--unknown"},{"--data-root",explicitDir.string(),"--data-root",explicitDir.string()}})
        require(prepareLaunch(args,"",choose,bundled.string()).action==LaunchAction::ArgumentError&&choices==0,"invalid arguments never select root");
    require(prepareLaunch({"--help"},"relative",choose,"relative").action==LaunchAction::Help&&choices==0,"help does not configure resources");
    p=prepareLaunch({},"",choose);require(p.action==LaunchAction::Start&&!p.bundledSelfAuthored&&choices==1,"legacy explicit chooser still works when no bundle default supplied");
    p=prepareLaunch({},"",[] {return std::string{};});require(p.action==LaunchAction::Cancelled&&!p.root.configured(),"legacy cancel ends startup");
    require(prepareLaunch({},"",{}).action==LaunchAction::RootError,"headless without config never searches cwd");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"no_picker_default\":true,\"explicit_precedence\":true,\"devices\":false}\n";
}
