module AlphaPoly(n) {
  // A
  if(n == 1) difference(){
            polygon([[0,0], [3.25,10], [4.75,10], [8,0], [6.375,0],
                    [5.625,2.5], [2.375,2.5], [1.625,0]]);
            polygon([[2.875,4],[4,7.5],[5.125,4]]);
        }
  // O
  if(n == 15) difference(){
			translate([4,5]) scale([4/5,1]) circle(r=5);
			translate([4,5]) scale([3.5/5,3.75/4]) circle(r=3.5);
		}
}

module ExtrudedPoly(n) {
  translate([-50, -50, -75]) linear_extrude(height = 150, center = false, convexity = 20) AlphaPoly(n);
}

module RotatedX(n, rz, rx) {
  rotate(a = (2*rx-1)*90, v = [1, 0, 0]) rotate(a = rz*90, v = [0, 0, 1]) ExtrudedPoly(n);
}

module RotatedY(n, rz, ry) {
  rotate(a = (2*ry-1)*90, v = [0, 1, 0]) rotate(a = rz*90, v = [0, 0, 1]) ExtrudedPoly(n);
}

module TripLet(a, b, c, rz1, ry, rz2, rx) {
  translate([0, 0, 0]) intersection() {
    ExtrudedPoly(a);
    RotatedY(b, rz1, ry);
    RotatedX(c, rz2, rx);
  }
}

$vpt = [-44.29, -45.99, -42.40];
$vpr = [16, 29, 8.4];
$vpd = 27;
color("MediumTurquoise",0.9) TripLet(1, 15, 0, 0, 0, 0, 0);
