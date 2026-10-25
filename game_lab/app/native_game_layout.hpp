// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
namespace game_lab {
struct NativeGameRect {double x=0,y=0,width=0,height=0;};
// Logical reference: 1024x768. Original UI records 1221/1222 form the
// 798x40/798x8 map borders; 1223/1224 form the 226x458/226x310 side.
// Local original-window template/edge measurements confirm their placement.
// Rectangles use Cocoa's bottom-left point convention, independent of Retina.
struct NativeGameLayout {
    double width=1024,height=768;
    NativeGameRect map,top,bottom,sideUpper,sideLower;
    NativeGameRect category(unsigned row) const {
        if(row>=11)return {};
        return {width-220,height-76.-38.*row,46,38};
    }
    NativeGameRect minimap() const {return {width-171,height-653,111,111};}
    NativeGameRect tool(unsigned slot) const {
        if(slot>=6)return {};
        // Local original selectors: 3 columns, 2 rows; edge uncertainty +/-1 point.
        return {width-168.+54.*(slot%3),height-371.-54.*(slot/3),54,53};
    }
    // Native action cells are deliberately separate from the original selector
    // slots: browse is always available, the category action sits next to it and
    // cancel never shares a row with demolition confirmation.
    NativeGameRect toolFor(unsigned tool) const {
        switch(tool){case 0:return this->tool(0);case 1:case 2:case 3:case 4:return this->tool(1);case 5:return this->tool(3);}
        return {};
    }
    NativeGameRect toolCancel() const {return tool(2);}
    NativeGameRect fileNew() const {return {4,height-34,84,24};}
    NativeGameRect fileQuit() const {return {92,height-34,52,24};}
    NativeGameRect resources() const {return {186,height-33,600,24};}
    NativeGameRect compactState() const {return {width-218,height-31,210,24};}
    NativeGameRect details() const {return {width-168,height-202,158,166};}
    NativeGameRect detailIcon() const {return {width-168,height-60,16,16};}
    NativeGameRect summary() const {return {width-168,height-290,158,80};}
    NativeGameRect houseConditions() const {return {width-168,height-318,64,28};}
    NativeGameRect detailDemolish() const {return {width-104,height-318,94,28};}
    NativeGameRect notice() const {return {width-168,height-456,158,24};}
    NativeGameRect pause() const {return {width-218,height-524,50,28};}
    NativeGameRect resume() const {return {width-164,height-524,50,28};}
    NativeGameRect speed(unsigned index) const {
        if(index>=3)return {};
        return {width-110.+36.*index,height-524,34,28};
    }
    NativeGameRect waterToggle() const {return {width-218,height-494,88,26};}
    NativeGameRect huntingToggle() const {return {width-124,height-494,116,26};}
    NativeGameRect demolitionConfirm() const {return {width-218,height-690,124,28};}
    NativeGameRect zoomOut() const {return {width-218,height-724,48,28};}
    NativeGameRect zoomIn() const {return {width-164,height-724,48,28};}
    NativeGameRect save() const {return {width-110,height-724,102,28};}
    NativeGameRect load() const {return {width-218,height-758,102,28};}
    NativeGameRect home() const {return {width-110,height-758,102,28};}
};
inline NativeGameLayout nativeGameLayout(double width,double height){
    NativeGameLayout l;
    if(std::isfinite(width))l.width=std::clamp(std::floor(width),1024.,2048.);
    if(std::isfinite(height))l.height=std::clamp(std::floor(height),768.,2048.);
    l.map={0,8,l.width-226,l.height-48};
    l.top={0,l.height-40,l.width-226,40};l.bottom={0,0,l.width-226,8};
    l.sideUpper={l.width-226,l.height-458,226,458};
    l.sideLower={l.width-226,0,226,l.height-458};
    return l;
}
} // namespace game_lab
