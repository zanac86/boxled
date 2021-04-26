$fn=180;

base_width=38;
base_deep=10;

board_width=18;
board_height=34;
board_deep=7;

pinstage_width=10.4;
pinstage_height=34;
pinstage_deep=3;
pinstage_shift=2;

leds_radius=32;
leds_deep=3;

mr=3;

difference()
{
    translate([-base_width/2+mr,-base_width/2+mr,0.5])
    {
        minkowski()
        {
            cube([base_width-mr*2, base_width-mr*2, base_deep-1]);
            cylinder(r=mr, h=1, center=true);
        }
    }

//    translate([-base_width/2,-base_width/2,0])
//    {
//        cube([base_width, base_width, base_deep]);
//    }
    
    translate([-board_width/2,-board_height/2, base_deep-board_deep])
    {
        cube([board_width, board_height, board_deep]);
    }

    translate([-pinstage_width/2, -pinstage_height/2+pinstage_shift, base_deep-board_deep-pinstage_deep+1])
    {
        cube([pinstage_width, pinstage_height, pinstage_deep]);
    }

    translate([0,0,base_deep-leds_deep])
    {
        cylinder(h=leds_deep, d=leds_radius);
    }
}


