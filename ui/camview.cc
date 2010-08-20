#define __STDC_FORMAT_MACROS

#define GL_GLEXT_PROTOTYPES
#define GL_ARB_IMAGING
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#include <stdexcept>
#include <cstring>
#include <gtkmm/accelmap.h>
#include "cameramonitor.h"
#include "main.h"

using namespace std;
using namespace Gtk;

void CameraMonitor::force_update() {
	do_histo_update();
	image.queue_draw();
}

const char *vertexprogram =
"void main() {"
"	gl_FrontColor = gl_Color;"
"	gl_TexCoord[0] = gl_MultiTexCoord0;"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
"}";

const char *fragmentprogram1 = 
"uniform sampler2D tex;"
"uniform vec4 scale;"
"uniform float bias;"
"uniform float underover;"
"void main() {"
"       vec4 color = texture2D(tex, vec2(gl_TexCoord[0]));"
"	color.gb -= underover * step(0.98, color.r);"
"	color.g += underover * step(color.r, 0.02);"
"	gl_FragColor = (color * scale) + bias;"
"}";

const char *fragmentprogram2 = 
"void main() {"
"	gl_FragColor = gl_Color;"
"}";

static GLuint program1 = 0;
static GLuint program2 = 0;

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} GLvec3f;

typedef struct {
	GLfloat u;
	GLfloat v;
	GLfloat nx;
	GLfloat ny;
	GLfloat nz;
	GLfloat x;
	GLfloat y;
	GLfloat z;
} GLt2n3v3f;

void CameraMonitor::on_image_realize() {
	fprintf(stderr, "REALIZATION!\n");

	glwindow = image.get_gl_window();
	if (!glwindow || !glwindow->gl_begin(image.get_gl_context()))
		return;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindTexture(GL_TEXTURE_2D, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glEnable(GL_FRAGMENT_SHADER);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glEnable(GL_FRAGMENT_SHADER_ARB);

	glEnable(GL_VERTEX_SHADER);
	glEnable(GL_VERTEX_PROGRAM_ARB);
	glEnable(GL_VERTEX_SHADER_ARB);

	static GLuint vertexshader = 0;
	static GLuint fragmentshader1 = 0;
	static GLuint fragmentshader2 = 0;

	fprintf(stderr, "compiling:\n%s\n", vertexprogram);
	vertexshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexshader, 1, (const GLchar **)&vertexprogram, 0);
	glCompileShader(vertexshader);
	GLint compiled = 0;
	glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
		throw runtime_error("Could not compile vertex shader!");

	fprintf(stderr, "compiling:\n%s\n", fragmentprogram1);
	fragmentshader1 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentshader1, 1, (const GLchar **)&fragmentprogram1, 0);
	glCompileShader(fragmentshader1);
	glGetShaderiv(fragmentshader1, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
		throw runtime_error("Could not compile fragment shader!");

	fprintf(stderr, "compiling:\n%s\n", fragmentprogram2);
	fragmentshader2 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentshader2, 1, (const GLchar **)&fragmentprogram2, 0);
	glCompileShader(fragmentshader2);
	glGetShaderiv(fragmentshader2, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
		throw runtime_error("Could not compile fragment shader!");

	fprintf(stderr, "linking...\n");
	program1 = glCreateProgram();
	glAttachShader(program1, vertexshader);
	glAttachShader(program1, fragmentshader1);
	glLinkProgram(program1);
	GLint linked = 0;
	glGetProgramiv(program1, GL_LINK_STATUS, &linked);
	if(!linked)
		throw runtime_error("Could not link shader!");

	fprintf(stderr, "linking...\n");
	program2 = glCreateProgram();
	glAttachShader(program2, vertexshader);
	glAttachShader(program2, fragmentshader2);
	glLinkProgram(program2);
	linked = 0;
	glGetProgramiv(program2, GL_LINK_STATUS, &linked);
	if(!linked)
		throw runtime_error("Could not link shader!");

	glwindow->gl_end();
}

void CameraMonitor::on_image_configure_event(GdkEventConfigure *event) {
	do_update();
}

void CameraMonitor::on_image_expose_event(GdkEventExpose *event) {
	do_update();
}

bool CameraMonitor::on_image_scroll_event(GdkEventScroll *event) {
	if(event->direction == GDK_SCROLL_UP)
		on_zoomin_activate();
	else if(event->direction == GDK_SCROLL_DOWN)
		on_zoomout_activate();

	return true;
}

static double clamp(double val, double min, double max) {
	return val < min ? min : val > max ? max : val;
}

bool CameraMonitor::on_image_motion_event(GdkEventMotion *event) {
	double s = pow(2.0, scale.get_value());
	fit.set_active(false);
	sx = clamp(sxstart + 2 * (event->x - xstart) / s / camera.get_width(), -1, 1);
	sy = clamp(systart - 2 * (event->y - ystart) / s / camera.get_height(), -1, 1);
	image.queue_draw();
	return true;
}

bool CameraMonitor::on_image_button_event(GdkEventButton *event) {
 	if(event->type == GDK_2BUTTON_PRESS) {
		fit.set_active(false);
		double s = pow(2.0, scale.get_value());
		sx = clamp(sxstart + 2 * (image.get_width() / 2 - xstart) / s / camera.get_width(), -1, 1);
		sy = clamp(systart - 2 * (image.get_height() / 2 - ystart) / s / camera.get_height(), -1, 1);
	} else if(event->type == GDK_3BUTTON_PRESS) {
		on_zoomin_activate();
		on_zoomin_activate();
		on_zoomin_activate();
	}
		
	sxstart = sx;
	systart = sy;
	xstart = event->x;
	ystart = event->y;
	return true;
}

void CameraMonitor::on_update() {
	if(!glwindow || !glwindow->gl_begin(image.get_gl_context()))
		return;
	{
		pthread::mutexholder h(&camera.monitor.mutex);

		dx = camera.monitor.dx;
		dy = camera.monitor.dy;
		depth = camera.monitor.depth;

		GLfloat scale = 1 << ((depth <= 8 ? 8 : 16) - depth);
		glPixelTransferf(GL_RED_SCALE, scale);
		glPixelTransferf(GL_GREEN_SCALE, scale);
		glPixelTransferf(GL_BLUE_SCALE, scale);

		if(camera.monitor.scale != s) {
			s = camera.monitor.scale;
			glTexImage2D(GL_TEXTURE_2D, 0, depth <= 8 ? GL_LUMINANCE8 : GL_LUMINANCE16, camera.get_width() / s, camera.get_height() / s, 0, GL_LUMINANCE, depth <= 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT, NULL);
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, camera.monitor.x1, camera.monitor.y1, camera.monitor.x2 - camera.monitor.x1, camera.monitor.y2 - camera.monitor.y1, GL_LUMINANCE, depth <= 8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT, camera.monitor.image);

		if(camera.monitor.histogram) {
			if(!histo)
				histo = (uint32_t *)malloc((1 << depth) * sizeof *histo);
			memcpy(histo, camera.monitor.histogram, (1 << depth) * sizeof *histo);
		} else if(histo) {
			free(histo);
			histo = 0;
		}
	}

	glwindow->gl_end();

	waitforupdate = false;
	force_update();
}

void CameraMonitor::do_update() {
	if(!glwindow || !glwindow->gl_begin(image.get_gl_context()))
		return;

	// Set up viewport

	double ww = image.get_width();
	double wh = image.get_height();
	double mwh = min(ww, wh);

	glViewport(0, 0, ww, wh);

	// Image scaling and centering

	double cw = camera.get_width();
	double ch = camera.get_height();
	double s;
	if(fit.get_active()) {
		s = min(ww / cw, wh / ch);
		scale.set_value(log(s)/log(2.0));
		sx = sy = 0;
	} else {
		s = pow(2.0, scale.get_value());
	}

	glLoadIdentity();
	glScalef(s * cw / ww, s * ch / wh, 1);

	// Render camera image

	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();
	glTranslatef(sx, sy, 0);
	if(fliph.get_active())
		glScalef(-1, 1, 1);
	if(flipv.get_active())
		glScalef(1, -1, 1);
	if(tiptilt.get_active() && isfinite(dx) && isfinite(dy))
		glTranslatef(2.0 * dx / camera.get_width(), -2.0 * dy / camera.get_height(), 0);

	glUseProgram(program1);
	GLint scale = glGetUniformLocation(program1, "scale");
	GLint bias = glGetUniformLocation(program1, "bias");
	GLint uo = glGetUniformLocation(program1, "underover");
	float min = minval.get_value();
	float max = maxval.get_value();
	float sc = (1 << depth) / (max - min);
	glUniform4f(scale, sc * camera.r, sc * camera.g, sc * camera.b, 1.0);
	glUniform1f(bias, -min / (max - min));
	glUniform1f(uo, 10 * underover.get_active());
	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (s == 1 || s >= 2) ? GL_NEAREST : GL_LINEAR);

	static const GLt2n3v3f rect[4] = {
		{0, 1, 0, 0, 1, -1, -1, 0},
		{1, 1, 0, 0, 1, +1, -1, 0},
		{0, 0, 0, 0, 1, -1, +1, 0},
		{1, 0, 0, 0, 1, +1, +1, 0},
	};
	glInterleavedArrays(GL_T2F_N3F_V3F, 0, rect);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glPopMatrix();

	glUseProgram(program2);
	glDisable(GL_TEXTURE_2D);

	// Render crosshair

	if(crosshair.get_active()) {
		glPushMatrix();
		glTranslatef(sx, sy, 0);
		glDisable(GL_LINE_SMOOTH);
		static const GLvec3f cross[4] = {
			{-1, 0, 0},
			{+1, 0, 0},
			{0, -1, 0},
			{0, +1, 0},
		};
		glColor3f(0, 1, 0);
		glVertexPointer(3, GL_FLOAT, 0, &cross);
		glDrawArrays(GL_LINES, 0, 4);
		glPopMatrix();
	}

	// Render pager

	if(s * cw > ww || s * ch > wh) {
		glPushMatrix();
		// Reset coordinate system
		glLoadIdentity();
		// Scale to approx. 10%
		double mcwh = std::min(cw, ch);
		glTranslatef(0.85, -0.85, 0);
		glScalef(0.1 * cw / mcwh / ww * mwh, 0.1 * ch / mcwh / wh * mwh, 1);

		// Draw pager bounding box
		static const GLvec3f box[4] = {
			{-1, -1, 0},
			{+1, -1, 0},
			{+1, +1, 0},
			{-1, +1, 0},
		};

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0, 1, 1, 0.5);
		glVertexPointer(3, GL_FLOAT, 0, &box);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		
		// Draw currently displayed image region
		glTranslatef(-sx, -sy, 0);
		glScalef(ww / cw / s, wh / ch / s, 1);

		glColor3f(0, 1, 1);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glPopMatrix();
		glDisable(GL_BLEND);
	}

	// Swap buffers.
	if (glwindow->is_double_buffered())
		glwindow->swap_buffers();
	else
		glFlush();

	glwindow->gl_end();
}

void CameraMonitor::do_histo_update() {
	// Calculate mean and stddev
	
	int pixels = 0;
	int max = 1 << depth;
	double sum = 0;
	double sumsquared = 0;
	double rms = max;
	bool overexposed = false;

	if(histo) {
		for(int i = 0; i < max; i++) {
			pixels += histo[i];
			sum += (double)i * histo[i];
			sumsquared += (double)(i * i) * histo[i];

			if(i >= 0.98 * max && histo[i])
				overexposed = true;
		}

		sum /= pixels;
		sumsquared /= pixels;
		rms = sqrt(sumsquared - sum * sum) / sum;
	}

	minval.set_range(0, 1 << depth);
	maxval.set_range(0, 1 << depth);
	mean.set_text(format("%.2lf", sum));
	stddev.set_text(format("%.3lf", rms));

	// Update min/max if necessary
	
	if(contrast.get_active()) {
		minval.set_value(sum - 5 * sum * rms);
		maxval.set_value(sum + 5 * sum * rms);
	}

	// Draw the histogram

	if(!histogramframe.is_visible())
		return;

	const int hscale = 1 + 10 * pixels / max;

	// Are we overexposed?

	if(overexposed && underover.get_active() && lastupdate & 1)
		histogrampixbuf->fill(0xff000000);
	else
		histogrampixbuf->fill(0xffffff00);

	uint8_t *out = (uint8_t *)histogrampixbuf->get_pixels();

	if(histo) {
		for(int i = 0; i < max; i++) {
			int height = histo[i] * 100 / hscale;
			if(height > 100)
				height = 100;
			for(int y = 100 - height; y < 100; y++) {
				uint8_t *p = out + 3 * ((i * 256 / max) + 256 * y);
				p[0] = 0;
				p[1] = 0;
				p[2] = 0;
			}
		}
	}

	int x1 = minval.get_value_as_int() * 256 / max;
	int x2 = maxval.get_value_as_int() * 256 / max;
	if(x1 > 255)
		x1 = 255;
	if(x2 > 255)
		x2 = 255;

	for(int y = 0; y < 100; y += 2) {
		uint8_t *p = out + 3 * (x1 + 256 * y);
		p[0] = 255;
		p[1] = 0;
		p[2] = 0;
		p = out + 3 * (x2 + 256 * y);
		p[0] = 0;
		p[1] = 255;
		p[2] = 255;
	}
	
	histogramimage.queue_draw();
}


bool CameraMonitor::on_timeout() {
	if(waitforupdate && time(NULL) - lastupdate < 5)
		return true;

	auto frame = get_window();
	if(!frame || frame->get_state() == Gdk::WINDOW_STATE_WITHDRAWN || frame->get_state() == Gdk::WINDOW_STATE_ICONIFIED)
		return true;

	int x1, y1, x2, y2;

	double cw = camera.get_width();
	double ch = camera.get_height();
	double ww = image.get_width();
	double wh = image.get_height();
	double ws = fit.get_active() ? min(ww / cw, wh / ch) : pow(2.0, scale.get_value());
	int cs = round(pow(2.0, -scale.get_value()));

	// Ensure camera scale results in a texture width divisible by 4
	while(cs > 1 && ((int)cw / cs) & 0x3)
		cs--;
	if(cs < 1)
		cs = 1;

	int fx = fliph.get_active() ? -1 : 1;
	int fy = flipv.get_active() ? -1 : 1;

	// Convert window corners to camera coordinates, use 4 pixel safety margin
	x1 = (cw - ww / ws - fx * sx * cw) / 2 / cs - 4;
	y1 = (ch - wh / ws + fy * sy * ch) / 2 / cs - 4;
	x2 = (cw + ww / ws - fx * sx * cw) / 2 / cs + 7;
	y2 = (ch + wh / ws + fy * sy * ch) / 2 / cs + 4;

	// Align x coordinates to multiples of 4
	x1 &= ~0x3;
	x2 &= ~0x3;

	waitforupdate = true;
	lastupdate = time(NULL);
	camera.grab(x1, y1, x2, y2, cs, darkflat.get_active(), fsel.get_active() ? 10 : 0);

	return true;
}

bool CameraMonitor::on_histogram_clicked(GdkEventButton *event) {
	if(event->type != GDK_BUTTON_PRESS)
		return false;

	int shift = depth - 8;
	if(shift < 0)
		shift = 0;
	double value = event->x * (1 << shift);
	
	if(event->button == 1)
		minval.set_value(value);
	if(event->button == 2) {
		minval.set_value(value - (16 << shift));
		maxval.set_value(value + (16 << shift));
	}
	if(event->button == 3)
		maxval.set_value(value);

	contrast.set_active(false);
	force_update();

	return true;
}

void CameraMonitor::on_zoom1_activate() {
	fit.set_active(false);
	scale.set_value(0);
	force_update();
}

void CameraMonitor::on_zoomin_activate() {
	fit.set_active(false);
	scale.spin(SPIN_STEP_FORWARD, 1.0/3.0);
	force_update();
}

void CameraMonitor::on_zoomout_activate() {
	fit.set_active(false);
	scale.spin(SPIN_STEP_BACKWARD, 1.0/3.0);
	force_update();
}

void CameraMonitor::on_histogram_toggled() {
	int w, h, fh;
	get_size(w, h);

	if(histogram.get_active()) {
		histogramframe.show();
		fh = histogramframe.get_height();
		resize(w, h + fh);
	} else {
		fh = histogramframe.get_height();
		histogramframe.hide();
		resize(w, h - fh);
	}
}

bool CameraMonitor::on_window_state_event(GdkEventWindowState *event) {
	config.set(camera.name + ".state", event->new_window_state);
	return true;
}

void CameraMonitor::on_window_configure_event(GdkEventConfigure *event) {
	if(get_window()->get_state())
		return;

	config.set(camera.name + ".x", event->x);
	config.set(camera.name + ".y", event->y);
	config.set(camera.name + ".w", event->width);
	config.set(camera.name + ".h", event->height);
}

void CameraMonitor::on_close_activate() {
	hide();
}

void CameraMonitor::on_fullscreen_toggled() {
	if(fullscreentoggle.get_active())
		fullscreen();
	else
		unfullscreen();
	force_update();
}

void CameraMonitor::on_colorsel_activate() {
	Gdk::Color color;
	ColorSelectionDialog dialog(camera.name + " image color");
	ColorSelection *selection = dialog.get_colorsel();

	color.set_rgb_p(camera.r, camera.g, camera.b);
	selection->set_current_color(color);

	if(dialog.run() != RESPONSE_OK)
		return;
	
	color = selection->get_current_color();
	camera.r = color.get_red_p();
	camera.g = color.get_green_p();
	camera.b = color.get_blue_p();
	force_update();
}

CameraMonitor::CameraMonitor(Camera &camera):
histogramframe("Histogram"),
histogramalign(0.5, 0.5, 0, 0),
scale("Scale down", "times"),
minval("Display min"), maxval("Display max"),
mean("Mean value"), stddev("Std. deviation"),
view("View"),
extra("Extra"),
fliph("Flip horizontal"),
flipv("Flip vertical"),
fit("Fit to window"),
zoom1("Zoom 1:1"),
zoomin("Zoom in"),
zoomout("Zoom out"),
histogram("Show histogram"),
contrast("Auto contrast"),
underover("Show under/overexposure"),
crosshair("Show crosshair"),
fullscreentoggle("Fullscreen"),
close(Stock::CLOSE),
colorsel("Image color"),
darkflat("Dark/flat correction"),
fsel("Frame selection"),
tiptilt("Soft tip/tilt"),
camera(camera) {
	lastupdate = 0;
	waitforupdate = false;
	histo = 0;
	s = -1;

	// widget properties
	
	set_title("Camera " + camera.name);

	glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGBA | Gdk::GL::MODE_DOUBLE);
	if(!glconfig) {
		fprintf(stderr, "Not double-buffered!\n");
		glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGBA);
	}
	if(!glconfig)
		throw runtime_error("Could not create OpenGL-capable visual");

	image.set_gl_capability(glconfig);
	image.set_double_buffered(false);
	sx = sy = 0;

	histogrampixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, 256, 100);
	histogrampixbuf->fill(0xFFFFFF00);
	histogramimage.set(histogrampixbuf);
	histogramimage.set_double_buffered(false);

	fliph.set_active(config.getbool(camera.name + ".fliph", false));
	flipv.set_active(config.getbool(camera.name + ".flipv", false));
	fit.set_active(config.getbool(camera.name + ".fit", true));
	histogram.set_active(config.getbool(camera.name + ".histogram", false));
	contrast.set_active(config.getbool(camera.name + ".contrast", false));
	underover.set_active(config.getbool(camera.name + ".underover", false));
	crosshair.set_active(config.getbool(camera.name + ".crosshair", false));

	camera.r = config.getdouble(camera.name + ".r", 1);
	camera.g = config.getdouble(camera.name + ".g", 1);
	camera.b = config.getdouble(camera.name + ".b", 1);
	darkflat.set_active(config.getbool(camera.name + ".darkflat", false));
	fsel.set_active(config.getbool(camera.name + ".fsel", false));
	tiptilt.set_active(config.getbool(camera.name + ".tiptilt", false));

	viewmenu.set_accel_group(get_accel_group());
	extramenu.set_accel_group(get_accel_group());

	AccelMap::add_entry("<camera>/menu/view/fliph", AccelKey("h").get_key(), Gdk::SHIFT_MASK);
	AccelMap::add_entry("<camera>/menu/view/flipv", AccelKey("v").get_key(), Gdk::SHIFT_MASK);
	AccelMap::add_entry("<camera>/menu/view/fit", AccelKey("f").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/zoom1", AccelKey("1").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/zoomin", AccelKey("plus").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/zoomout", AccelKey("minus").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/histogram", AccelKey("h").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/contrast", AccelKey("c").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/underover", AccelKey("e").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/view/crosshair", AccelKey("c").get_key(), Gdk::SHIFT_MASK);
	AccelMap::add_entry("<camera>/menu/view/fullscreen", AccelKey("F11").get_key(), Gdk::ModifierType(0));
	
	AccelMap::add_entry("<camera>/menu/extra/darkflat", AccelKey("d").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/extra/fsel", AccelKey("s").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camera>/menu/extra/tiptilt", AccelKey("t").get_key(), Gdk::ModifierType(0));

	fliph.set_accel_path("<camera>/menu/view/fliph");
	flipv.set_accel_path("<camera>/menu/view/flipv");
	fit.set_accel_path("<camera>/menu/view/fit");
	zoom1.set_accel_path("<camera>/menu/view/zoom1");
	zoomin.set_accel_path("<camera>/menu/view/zoomin");
	zoomout.set_accel_path("<camera>/menu/view/zoomout");
	histogram.set_accel_path("<camera>/menu/view/histogram");
	contrast.set_accel_path("<camera>/menu/view/contrast");
	underover.set_accel_path("<camera>/menu/view/underover");
	crosshair.set_accel_path("<camera>/menu/view/crosshair");
	fullscreentoggle.set_accel_path("<camera>/menu/view/fullscreen");
	darkflat.set_accel_path("<camera>/menu/extra/darkflat");
	fsel.set_accel_path("<camera>/menu/extra/fsel");
	tiptilt.set_accel_path("<camera>/menu/extra/tiptilt");

	depth = config.getint(camera.name + ".depth", camera.get_depth());

	minval.set_range(0, 1 << depth);
	minval.set_digits(0);
	minval.set_increments(1, 16);
	minval.set_value(config.getint(camera.name + ".minval", 0));
	maxval.set_range(0, 1 << depth);
	maxval.set_digits(0);
	maxval.set_increments(1, 16);
	maxval.set_value(config.getint(camera.name + ".maxval", 1 << depth));

	scale.set_range(-3, 3);
	scale.set_digits(3);
	scale.set_increments(1.0/3.0, 1.0/3.0);
	scale.set_value(config.getint(camera.name + ".scale", 1));

	mean.set_alignment(1);
	stddev.set_alignment(1);

	mean.set_editable(false);
	stddev.set_editable(false);

	set_gravity(Gdk::GRAVITY_STATIC);

	// signals

	signal_configure_event().connect_notify(sigc::mem_fun(*this, &CameraMonitor::on_window_configure_event));
	image.signal_configure_event().connect_notify(sigc::mem_fun(*this, &CameraMonitor::on_image_configure_event));
	image.signal_expose_event().connect_notify(sigc::mem_fun(*this, &CameraMonitor::on_image_expose_event));
	image.signal_realize().connect(sigc::mem_fun(*this, &CameraMonitor::on_image_realize));
	imageevents.signal_scroll_event().connect(sigc::mem_fun(*this, &CameraMonitor::on_image_scroll_event));
	imageevents.signal_button_press_event().connect(sigc::mem_fun(*this, &CameraMonitor::on_image_button_event));
	imageevents.signal_motion_notify_event().connect(sigc::mem_fun(*this, &CameraMonitor::on_image_motion_event));
	imageevents.add_events(Gdk::POINTER_MOTION_HINT_MASK);
	
	camera.signal_monitor.connect(sigc::mem_fun(*this, &CameraMonitor::on_update));
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &CameraMonitor::on_timeout), 1000.0/30.0);

	fliph.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::force_update));
	flipv.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::force_update));
	fit.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::force_update));
	contrast.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::force_update));
	underover.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::force_update));
	crosshair.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::force_update));

	histogram.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::on_histogram_toggled));
	zoom1.signal_activate().connect(sigc::mem_fun(*this, &CameraMonitor::on_zoom1_activate));
	zoomin.signal_activate().connect(sigc::mem_fun(*this, &CameraMonitor::on_zoomin_activate));
	zoomout.signal_activate().connect(sigc::mem_fun(*this, &CameraMonitor::on_zoomout_activate));
	histogramevents.signal_button_press_event().connect(sigc::mem_fun(*this, &CameraMonitor::on_histogram_clicked));
	fullscreentoggle.signal_toggled().connect(sigc::mem_fun(*this, &CameraMonitor::on_fullscreen_toggled));
	close.signal_activate().connect(sigc::mem_fun(*this, &CameraMonitor::on_close_activate));
	colorsel.signal_activate().connect(sigc::mem_fun(*this, &CameraMonitor::on_colorsel_activate));


	// layout
	
	viewmenu.add(fliph);
	viewmenu.add(flipv);
	viewmenu.add(tsep1);
	viewmenu.add(fit);
	viewmenu.add(zoom1);
	viewmenu.add(zoomin);
	viewmenu.add(zoomout);
	viewmenu.add(tsep2);
	viewmenu.add(histogram);
	viewmenu.add(contrast);
	viewmenu.add(tsep3);
	viewmenu.add(underover);
	viewmenu.add(crosshair);
	viewmenu.add(tsep4);
	viewmenu.add(fullscreentoggle);
	viewmenu.add(close);
	view.set_submenu(viewmenu);

	extramenu.add(colorsel);
	extramenu.add(darkflat);
	extramenu.add(fsel);
	extramenu.add(tiptilt);
	extra.set_submenu(extramenu);

	menubar.add(view);
	menubar.add(extra);

	histogramevents.add(histogramimage);
	histogramalign.add(histogramevents);
	histogramvbox.pack_start(histogramalign, PACK_SHRINK);
	histogramvbox.pack_start(mean, PACK_SHRINK);
	histogramvbox.pack_start(stddev, PACK_SHRINK);
	histogramvbox.pack_start(minval, PACK_SHRINK);
	histogramvbox.pack_start(maxval, PACK_SHRINK);
	histogramframe.add(histogramvbox);

	imageevents.add(image);

	vbox.pack_start(menubar, PACK_SHRINK);
	vbox.pack_start(imageevents);
	vbox.pack_start(histogramframe, PACK_SHRINK);

	add(vbox);

	// finalize

	show_all_children();

	if(!histogram.get_active())
		histogramframe.hide();

	if(config.exists(camera.name + ".w") && config.exists(camera.name + ".h"))
		resize(config.getint(camera.name + ".w"), config.getint(camera.name + ".h"));

	if(config.exists(camera.name + ".x") && config.exists(camera.name + ".y"))
		move(config.getint(camera.name + ".x"), config.getint(camera.name + ".y"));

	if(config.exists(camera.name + ".state")) {
		GdkWindowState state = (GdkWindowState)config.getint(camera.name + ".state");
		if(state & GDK_WINDOW_STATE_ICONIFIED)
			iconify();
		if(state & GDK_WINDOW_STATE_FULLSCREEN)
			fullscreentoggle.activate();
		if(state & GDK_WINDOW_STATE_MAXIMIZED)
			maximize();
		if(!(state & GDK_WINDOW_STATE_WITHDRAWN))
			present();
		if(state & GDK_WINDOW_STATE_STICKY)
			stick();
	}
}

CameraMonitor::~CameraMonitor() {
	config.set(camera.name + ".fliph", fliph.get_active());
	config.set(camera.name + ".flipv", flipv.get_active());
	config.set(camera.name + ".fit", fit.get_active());
	config.set(camera.name + ".histogram", histogram.get_active());
	config.set(camera.name + ".contrast", contrast.get_active());
	config.set(camera.name + ".underover", underover.get_active());
	config.set(camera.name + ".crosshair", crosshair.get_active());

	config.set(camera.name + ".r", camera.r);
	config.set(camera.name + ".g", camera.g);
	config.set(camera.name + ".b", camera.b);
	config.set(camera.name + ".darkflat", darkflat.get_active());
	config.set(camera.name + ".fsel", fsel.get_active());
	config.set(camera.name + ".tiptilt", tiptilt.get_active());

	config.set(camera.name + ".depth", camera.get_depth());
	config.set(camera.name + ".scale", scale.get_value_as_int());
	config.set(camera.name + ".minval", minval.get_value_as_int());
	config.set(camera.name + ".maxval", maxval.get_value_as_int());
}
