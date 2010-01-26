/*
 *  opengl-test.cc
 *  foam
 *
 *  Created by Tim on 20100126.
 *  Copyright 2010 Tim van Werkhoven. All rights reserved.
 *
 * Based on:
 * simple.cc:
 * Simple gtkglextmm example.
 *
 * written by Naofumi Yasufuku  <naofumi@users.sourceforge.net>
 */


#include <iostream>
#include <cstdlib>

#include <gtkmm.h>
#include <gtkglmm.h>

#ifdef HAVE_GL_GL_H
#include "GL/gl.h"
#elif HAVE_OPENGL_GL_H
#include "OpenGL/gl.h"
#endif

#ifdef HAVE_GL_GLU_H
#include "GL/glu.h"
#elif HAVE_OPENGL_GLU_H 
#include "OpenGL/glu.h"
#endif

#include <GL/glext.h>

//
// OpenGL frame buffer configuration utilities.
//

using namespace std;
using namespace Gtk;


//
// Simple OpenGL scene.
//

class Simplem_glscene : public Gtk::GL::DrawingArea
{

protected:
  virtual void on_realize();
  virtual bool on_configure_event(GdkEventConfigure* event);
  virtual bool on_expose_event(GdkEventExpose* event);
  GLuint TexObj[2];
  void draw();
public:
	GLubyte image[640 * 480];
  Simplem_glscene();
  virtual ~Simplem_glscene();
  void update_image();
	GLfloat angle;
	GLfloat zoom;
};

Simplem_glscene::Simplem_glscene() {	
  Glib::RefPtr<Gdk::GL::Config> glconfig;
	angle = 0.0;
	zoom = 1.0;
	
  // Try double-buffered visual
  glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGBA    |
                                     Gdk::GL::MODE_DOUBLE);
  if (!glconfig) {
		std::cerr << "*** Cannot find the double-buffered visual.\n"
		<< "*** Trying single-buffered visual.\n";
		
		// Try single-buffered visual
		glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGBA   |
																			 Gdk::GL::MODE_DEPTH);
		if (!glconfig)
		{
			std::cerr << "*** Cannot find any OpenGL-capable visual.\n";
			std::exit(1);
		}
	}

  set_gl_capability(glconfig);
  set_double_buffered(false);
}

Simplem_glscene::~Simplem_glscene() {}

void Simplem_glscene::draw() {
	// Clear screen first
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	//glTranslatef( -1.0, 0.0, 0.0 );
	// Rotate and scale the Modelview
	glRotatef( angle, 0.0, 0.0, 1.0 );
	glScalef( zoom, zoom, zoom );
	// Use texture TexObj[0]
	glBindTexture( GL_TEXTURE_2D, TexObj[0] );
	// Draw a square with a texture on it
	glBegin( GL_POLYGON );
		glTexCoord2f( 0.0, 0.0 );   glVertex2f( -1.0, -1.0 );
		glTexCoord2f( 1.0, 0.0 );   glVertex2f(  1.0, -1.0 );
		glTexCoord2f( 1.0, 1.0 );   glVertex2f(  1.0,  1.0 );
		glTexCoord2f( 0.0, 1.0 );   glVertex2f( -1.0,  1.0 );
	glEnd();
	glPopMatrix();
}

void Simplem_glscene::update_image() {
//	GLint histogram[256];
//	int i;
	fprintf(stderr, "Updating texture...\n");
	
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
	
  if (!glwindow->gl_begin(get_gl_context()))
    return;
	
	// Generate texture object
	//glGenTextures(1, &TexObj[0]);
	// Bind texture object to TexObj[0]
	glBindTexture( GL_TEXTURE_2D, TexObj[0] );
	// Load image as texture
	glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, 640, 480, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
//	glGetHistogram(GL_HISTOGRAM, GL_TRUE, GL_LUMINANCE, GL_UNSIGNED_INT, histogram);
//	for(i = 0; i < 32; i++)
//		cout << histogram[i] << " ";
//	cout << "\n";
	
  glwindow->gl_end();
	
	queue_draw();
}

void Simplem_glscene::on_realize() {
  // We need to call the base on_realize()
  Gtk::DrawingArea::on_realize();
	
	
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context()))
    return;
	
	fprintf(stderr, "Realizing...\n");
//	glDisable( GL_DITHER );
	
	glEnable( GL_TEXTURE_2D );
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
//	glEnable( GL_HISTOGRAM );
//	glHistogram(GL_HISTOGRAM, 256, GL_LUMINANCE, GL_FALSE);
//	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
//	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );
	
	glGenTextures( 1, TexObj );
	glBindTexture( GL_TEXTURE_2D, TexObj[0] );
	
//	glPixelTransferf(GL_RED_SCALE, 512);
//	glPixelTransferf(GL_GREEN_SCALE, 256);
//	glPixelTransferf(GL_BLUE_SCALE, 128);
//	glPixelTransferf(GL_ALPHA_SCALE, 512);
	
	glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, 640, 480, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
//	fprintf(stderr, "result = %d\n", glIsTexture(TexObj[0]));
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
//	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	
	//draw();
	
  glwindow->gl_end();
  // *** OpenGL END ***
}

bool Simplem_glscene::on_configure_event(GdkEventConfigure* event) {
	// Initialize OpenGL settings
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
	
  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context()))
    return false;
	
	// Use whole screen for OpenGL
  glViewport(0, 0, get_width(), get_height());
	
	// Reset projection matrix (camera perspective)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	/*   glOrtho( -3.0, 3.0, -3.0, 3.0, -10.0, 10.0 );*/
	//glFrustum( -1.0, 1.0, -1.0, 1.0, 6.0, 20.0 );
	// Reset model matrix (model rotations 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glTranslatef( 0.0, 0.0, -8.0 );
	
  glwindow->gl_end();
  // *** OpenGL END ***
	
  return true;
}

bool Simplem_glscene::on_expose_event(GdkEventExpose* event) {
	// Called when window needs to be redrawn
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context()))
    return false;
	
  draw();
	
  // Swap buffers.
  if (glwindow->is_double_buffered())
    glwindow->swap_buffers();
  else
    glFlush();
	
  glwindow->gl_end();
  // *** OpenGL END ***
	
  return true;
}

class Simple : public Gtk::Window {
protected:
  // signal handlers:
  void on_button_quit_clicked();
	void on_zoomin();
	void on_zoomout();
	void on_rotleft();
	void on_rotright();
	
  // member widgets:
  Gtk::VBox m_vbox;
	Gtk::HBox m_hbox;

	Gtk::ScrolledWindow scroll;
//	Gtk::Viewport vp;
//	Gtk::Adjustment h_adj;
//	Gtk::Adjustment v_adj;
  Simplem_glscene m_glscene;
  
	Gtk::Button m_zoomin;
  Gtk::Button m_zoomout;
	Gtk::Button m_rotleft;
	Gtk::Button m_rotright;
	Gtk::Button m_quit;
public:
	int m_update, m_update2;
  Glib::Dispatcher dispatcher;
	bool on_update();
	bool autorot();
  Simple();
  virtual ~Simple();
	
};

Simple::Simple(): 
m_vbox(false, 0), m_hbox(false, 0), 
scroll(),
m_zoomin("Zoom in"), m_zoomout("Zoom out"), m_rotleft("Rotate left"), m_rotright("Rotate right"), m_quit("Quit") {
  set_title("Simple");
	
  // Get automatically redrawn if any of their children changed allocation.
  set_reallocate_redraws(true);
	
  add(m_vbox);
	
	scroll.set_policy(POLICY_ALWAYS, POLICY_ALWAYS);
	scroll.set_size_request(256, 256);
	scroll.set_double_buffered(false);
  m_glscene.set_size_request(1000, 1000);
	scroll.add(m_glscene);
	//vp.add();
	
  m_vbox.pack_start(scroll);
	
	m_zoomin.signal_clicked().connect(sigc::mem_fun(*this, &Simple::on_zoomin));
	m_zoomout.signal_clicked().connect(sigc::mem_fun(*this, &Simple::on_zoomout));
	m_rotleft.signal_clicked().connect(sigc::mem_fun(*this, &Simple::on_rotleft));
	m_rotright.signal_clicked().connect(sigc::mem_fun(*this, &Simple::on_rotright));
  m_quit.signal_clicked().connect(sigc::mem_fun(*this, &Simple::on_button_quit_clicked));
	
	m_hbox.pack_start(m_zoomin);
	m_hbox.pack_start(m_zoomout);
	m_hbox.pack_start(m_rotleft);
	m_hbox.pack_start(m_rotright);
	m_hbox.pack_start(m_quit);
	
  m_vbox.pack_start(m_hbox, Gtk::PACK_SHRINK, 0);
	
	dispatcher.connect(sigc::mem_fun(&m_glscene, &Simplem_glscene::update_image));
	
	//m_update = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Simple::on_update), 1000 );
  show_all();
	
	on_update();
	m_update2 = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Simple::autorot), 100 );
}

Simple::~Simple() {}

void Simple::on_button_quit_clicked() {
  Gtk::Main::quit();
}

void Simple::on_zoomin() {
	std::cout << "on_zoomin()\n";
	m_glscene.zoom *= 1.1;
	dispatcher();
}

void Simple::on_zoomout() {
	std::cout << "on_zoomout()\n";
	m_glscene.zoom /= 1.1;
	dispatcher();
}

void Simple::on_rotleft() {
	std::cout << "on_rotleft()\n";
	m_glscene.angle -= 10;
	if (m_glscene.angle <= 360) m_glscene.angle += 360;
	dispatcher();
}

void Simple::on_rotright() {
	std::cout << "on_rotright()\n";
	m_glscene.angle += 10;
	if (m_glscene.angle >= 360) m_glscene.angle -= 360;
	dispatcher();
}

bool Simple::autorot() {
	m_glscene.angle += 10;
	if (m_glscene.angle >= 360) m_glscene.angle -= 360;	
	dispatcher();
	return true;
}

bool Simple::on_update() {
	std::cout << "on_update()\n";
	long i;
	
	for (i=0; i<640*480; i++) {
		m_glscene.image[i] = (GLubyte) (i * 255.0 / (640.*480.));
//		if (i % ((640*480)/10) == 0) printf("%ld:%d", i, image[i]);
	}
	printf("\n");
	
	
	dispatcher();
	
	return true;
}


int main(int argc, char** argv) {
  Gtk::Main kit(argc, argv);
  Gtk::GL::init(argc, argv);
	
  int major, minor;
  Gdk::GL::query_version(major, minor);
  std::cout << "OpenGL extension version - "
	<< major << "." << minor << std::endl;
	
  Simple simple;
	
  kit.run(simple);
	
  return 0;
}

