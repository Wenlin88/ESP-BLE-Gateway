#include "M5Atom.h"

int GRB_COLOR_WHITE = 0xffffff;
int GRB_COLOR_BLACK = 0x000000;
int GRB_COLOR_RED = 0xff0000; //0x00ff00; RED and Green mixed in original
int GRB_COLOR_ORANGE = 0xa5ff00;
int GRB_COLOR_YELLOW = 0xffff00;
int GRB_COLOR_GREEN = 0x00ff00; //0xff0000;
int GRB_COLOR_BLUE = 0x0000ff;
int GRB_COLOR_PURPLE = 0x008080;

int cross[25] = 
{
  1,0,0,0,1,
  0,1,0,1,0,
  0,0,1,0,0,
  0,1,0,1,0,
  1,0,0,0,1
};

int ok[25] = 
{
  0,0,0,0,0,
  0,1,1,1,0,
  0,1,0,1,0,
  0,1,1,1,0,
  0,0,0,0,0
};

int triang[25]=
{
  0,0,0,0,0,
  0,0,1,0,0,
  0,1,0,1,0,
  0,1,1,1,0,
  0,0,0,0,0
};

void draw_array(int arr[], int colors[])
{
  for(int i = 0; i < 25; i++)
  {
      M5.dis.drawpix(i, colors[arr[i]]);
  }
}


void draw_cross(){
    M5.dis.clear();
    int color_list[] = {GRB_COLOR_BLACK, GRB_COLOR_RED};
    draw_array(cross, color_list);
}

void draw_ok(){
    M5.dis.clear();
    int color_list[] = {GRB_COLOR_BLACK, GRB_COLOR_GREEN};
    draw_array(ok, color_list);
}

void draw_triangle(){
    M5.dis.clear();
    int color_list[] = {GRB_COLOR_BLACK, GRB_COLOR_BLUE};
    draw_array(triang, color_list);
}
