//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Varjú Ákos
// Neptun : SSNTUX
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char* const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	layout(location = 0) in vec4 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x/(vp.w+1), vp.y/(vp.w+1), 0, 1);		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char* const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders
const int baseCircle = 360; //kört aproximáló csúcsok száma
vec3 white(1, 1, 1);
vec3 black(0, 0, 0);
vec3 red(1, 0, 0);
vec3 green(0, 1, 0);
vec3 blue(0, 0, 1);

//Egy ponthoz képest egy másik pont irányának és távolságának meghatározása.
float skalarszorzat(vec4 p1, vec4 p2) {
	return p1.x * p2.x + p1.y * p2.y + p1.z * p2.z - p1.w * p2.w;
}

float length(vec4 v) { return sqrtf(skalarszorzat(v, v)); }

vec4 normalize(vec4 v) {
	return vec4(v * 1 / length(v));

}


vec4 perpendicularVector(const vec4& v, const vec4& p) {
	vec3 tmp1, tmp2, tmp3;
	tmp1.x = v.x; tmp1.y = v.y; tmp1.z = -v.w; //z = -w , z=0
	tmp2.x = p.x; tmp2.y = p.y; tmp2.z = -p.w;

	tmp3 = cross(tmp1, tmp2);

	return vec4(tmp3.x, tmp3.y, 0, tmp3.z);			//ell
}


//Adott pontból és sebesség vektorral induló pont HELYÉNEK számítása t idõvel késõbb.
vec4 PositionAfterTime(vec4 p, vec4 v, float t) {
	return vec4(p * cosh(t) + v * sinh(t));
}
//Adott pontból és sebesség vektorral induló pont SEBESSÉG vektorának számítása t idõvel késõbb.
vec4 VelocityAfterTime(vec4 p, vec4 v, float t) {
	return normalize(vec4(p * sinh(t) + v * cosh(t)));
}
//TÁVOLSÁG	 
float distance(vec4 p1, vec4 p2) {
	float d = acoshf(-skalarszorzat(p1, p2));
	return d;
}
//IRÁNY
vec4 PointDirection(vec4 p, vec4 q, float t) {
	float d = distance(p, q);	//két pont közötti távolság

	return (q - coshf(d) * p) / sinhf(d);
}
// Egy pontban egy vektor elforgatása adott szöggel.
vec4 Rotate(vec4 p, vec4 v, float fi) {
	vec4 tmp = perpendicularVector(p, v);
	return normalize(vec4(v * cosf(fi) + tmp * sinf(fi)));
}
//Egy közelítõ pont és sebességvektorhoz a geometria szabályait teljesítõ, közeli PONT és sebesség választása.
vec4 PontSikon(vec4 p) {
	p.w = sqrt(p.x * p.x + p.y * p.y + 1);
	return p;
}
vec4 VelocitySikon(vec4 p, vec4 v) {
	return normalize(v + skalarszorzat(p, v) * p);
}

class Circle {
public:
	float radius;
	vec3 color;
	vec4 center;
	vec4 velocity;

	unsigned int vao, vbo;

	Circle() {
	}

	Circle(float radius, vec3 color, vec4 c, vec4 v) {
		this->radius = radius;
		this->color = color;
		this->center = c;
		this->velocity = v;
	}

	void create() {
		glGenVertexArrays(1, &vao);	// get 1 vao id
		glGenBuffers(1, &vbo);	// Generate 1 buffer

		glBindVertexArray(vao);		// make it active
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//printf("%d\n", vbo);
		vec4 vertices[baseCircle];
		for (int i = 0; i < baseCircle; i++) {
			float fi = i * 2 * M_PI / baseCircle;
			vec4 normV = normalize(velocity);
			vec4 rotV = Rotate(center, normV, fi);
			vertices[i] = PositionAfterTime(center, rotV, radius);
		}

		glBufferData(GL_ARRAY_BUFFER, 	//átadni a GPU-nak
			sizeof(vec4) * baseCircle,  // # bytes
			vertices,	      	// address
			GL_STATIC_DRAW);	// we do not change later

		glEnableVertexAttribArray(0);  // AttribArray 0
		glVertexAttribPointer(0,       // vbo -> AttribArray 0
			4, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
			0, NULL); 		     // stride, offset: tightly packed
	}

	void draw() {
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, color.x, color.y, color.z); // 3 floats

		glBindVertexArray(vao);  // Draw call
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glDrawArrays(GL_TRIANGLE_FAN, 0 /*startIdx*/, baseCircle /*# Elements*/);
	}
	void setVelocity(vec4 velocity) {
		this->velocity = velocity;
	}
	void setPosition(vec4 center) {
		this->center = center;
	}
	vec4 getPosition() {
		return center;
	}
	vec4 getVelocity() {
		return velocity;
	}
	float getRadius() {
		return radius;
	}

};
float mradius = 0.05f;
float step = -0.001f;
class Ufo {
public:
	vec4 center;
	vec4 velocity;

	Circle body;
	Circle eye_ball1;
	Circle eye_ball2;
	Circle eye1;
	Circle eye2;
	Circle mouth;


	std::vector<vec4> points;
	unsigned int vaou, vbou;

	//Center  x : 1.444049 y : -0.733317 z : 0.000000 w : 1.903426
	//Velocity x : 1.680733 y : -0.888515 z : 0.000000 w : 1.616887
	//green(0,1,0)
	//red(1,0,0)
	Ufo(vec3 color) {
		
		points = std::vector<vec4>();
		center = PontSikon(vec4(0, 0, 0, 1));
		velocity = VelocitySikon(center, vec4(1, 0, 0, 0));
		if (color.y==1) {
			center = PontSikon(vec4(1.444049, -0.733317, 0, 1.903426));
			velocity = VelocitySikon(center, vec4(1.680733, -0.888515, 0, 1.616887));

		}
		//createUfo(color, center);

	}
	void createUfo(vec3 color, vec4 position) {
		vec4 v = PointDirection(center, position, distance(position, center));

		body = Circle(0.3f, color, center, velocity);

		eye_ball1 = Circle(0.1f, white, PontSikon(PositionAfterTime(center, Rotate(center, velocity, M_PI / 5.5), 0.3)), VelocitySikon(PontSikon(PositionAfterTime(center, Rotate(center, velocity, M_PI / 6), 0.3)), VelocityAfterTime(center, velocity, 0.3)));
		eye1 = Circle(0.05f, blue, PontSikon(PositionAfterTime(eye_ball1.center, v, 0.05)), VelocitySikon(PontSikon(PositionAfterTime(eye_ball1.center, v, 0.05)), VelocityAfterTime(eye_ball1.center, v, 0.3)));

		eye_ball2 = Circle(0.1f, white, PontSikon(PositionAfterTime(center, Rotate(center, velocity, -M_PI / 5.5), 0.3)), VelocitySikon(PontSikon(PositionAfterTime(center, Rotate(center, velocity, -M_PI / 6), 0.3)), VelocityAfterTime(center, velocity, 0.3)));
		eye2 = Circle(0.05f, blue, PontSikon(PositionAfterTime(eye_ball2.center, v, 0.05)), VelocitySikon(PontSikon(PositionAfterTime(eye_ball2.center, v, 0.05)), VelocityAfterTime(eye_ball2.center, v, 0.3)));
		
		mouth = Circle(mradius, black, PontSikon(PositionAfterTime(center, velocity, 0.3)), VelocitySikon(PontSikon(PositionAfterTime(center, velocity, 0.3)), VelocityAfterTime(center, velocity, 0.3)));

		body.create();
		eye_ball1.create();
		eye_ball2.create();
		eye1.create();
		eye2.create();
		mouth.create();
	}
	void draw() {
		body.draw();
		eye_ball1.draw();
		eye_ball2.draw();
		eye1.draw();
		eye2.draw();
		mouth.draw();
	}
	void setVelocity(vec4 velocity) {
		this->velocity = velocity;
		body.setVelocity(velocity);
		eye_ball1.setVelocity(velocity);
		eye_ball2.setVelocity(velocity);
		eye1.setVelocity(velocity);
		eye2.setVelocity(velocity);
		mouth.setVelocity(velocity);
	}
	void setPosition(vec4 center) {
		this->center = center;
		body.setPosition(center);
		eye_ball1.setPosition(center);
		eye_ball2.setPosition(center);
		eye1.setPosition(center);
		eye2.setPosition(center);
		mouth.setPosition(center);
	}
	vec4 getPosition() {
		return center;
	}
	vec4 getVelocity() {
		return velocity;
	}
	void mozog() {
		points.push_back(getPosition());
		vec4 position = PontSikon(PositionAfterTime(getPosition(), normalize(getVelocity()), 0.001f));
		vec4 velocity = VelocitySikon(center, VelocityAfterTime(getPosition(), normalize(getVelocity()), 0.001f));
		setPosition(position);
		setVelocity(velocity);
	}
	void forog1() {
		setVelocity(normalize(Rotate(getPosition(), getVelocity(), 0.001f)));
	}
	void forog2() {
		setVelocity(normalize(Rotate(getPosition(), getVelocity(), -0.001f)));
	} 

	void drawLine() {
		glGenVertexArrays(1, &vaou);
		glGenBuffers(1, &vbou);
		glBindBuffer(GL_ARRAY_BUFFER, vbou);
		glBindVertexArray(vaou);

		glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec4), points.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 1, 1, 1);



		glBindVertexArray(vaou);
		glLineWidth(4);
		glDrawArrays(GL_LINE_STRIP, 0, points.size());
	}
	void changeMouthRadius() {
		
		mradius += step; ;

		if (mradius >= 0.08 || mradius <= 0.02) {
			step = -step;
		}
	}



};


Circle circle(50.0f, vec3(0, 0, 0), vec4(0, 0, 0, 1.0f), vec4(1.0f, 0, 0, 0));
Ufo greenUfo(vec3(0,1,0));
Ufo redUfo(vec3(1,0,0));

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	circle.create();
	greenUfo.createUfo(green, redUfo.getPosition());
	redUfo.createUfo(red, greenUfo.getPosition());
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


void onDisplay() {
	glClearColor(0.5, 0.5, 0.5, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer
	circle.draw();
	greenUfo.drawLine();
	redUfo.drawLine();
	greenUfo.draw();
	redUfo.draw();


	glutSwapBuffers(); // exchange buffers for double buffering
}

bool keyPressed[256] = { false };
// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	keyPressed[key] = true;
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
	keyPressed[key] = false;
}


// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
}

float elapsed_time;
// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	for (int i = 0; i < time - elapsed_time; i++) {
		if (keyPressed['e']) {
			redUfo.mozog();
		}
		if (keyPressed['s']) {
			redUfo.forog1();
		}
		if (keyPressed['f']) {
			redUfo.forog2();
		}
		greenUfo.mozog();
		greenUfo.forog1();
		greenUfo.forog1();
	}
	greenUfo.changeMouthRadius();
	redUfo.changeMouthRadius();
	greenUfo.createUfo(green, redUfo.getPosition());
	redUfo.createUfo(red, greenUfo.getPosition());
	elapsed_time = time;
	glutPostRedisplay();
}



