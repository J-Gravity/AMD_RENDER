
#include <engine.h>
#include <string>

static void				grav_exit()
{
	if (g_render->ren)
		SDL_DestroyRenderer(g_render->ren);
	if (g_render->win)
		SDL_DestroyWindow(g_render->win);
	delete	g_render;
	SDL_Quit();
	exit(0);
}

template <class ttype>
static inline void		check_error(ttype check, std::string message)
{
	if (check == (ttype)0)
	{
		std::cerr << message;
		grav_exit();
	}
}

Render::Render(int w, int h, std::string path, int excl)
{
	this->width = w;
	this->height = h;
	this->path = path;
	this->scale = 2000000000000000.0;
	this->excl = excl;
	check_error(!SDL_Init(SDL_INIT_EVERYTHING), "SDL Init error\n");
	this->win = SDL_CreateWindow("J-Gravity", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
	check_error(this->win, "SDL Window creation error\n");
	this->ren = SDL_CreateRenderer(this->win, -1, SDL_RENDERER_ACCELERATED
		| SDL_RENDERER_PRESENTVSYNC);
	check_error(this->ren, "SDL Render create error\n");
	this->cam = new Camera(this->width, this->width,
		new Vector(0, 0, 0.0), new Matrix(ROTATION, 0.0, 0.0, 0.0),
		60, 1000.0, 500000.0);
	this->tick = 0;
	Color::set_range(1.0, 4.0);
	Color::init_color_table(RAINBOW, 128);
	init_threading();
}

static inline long		h_read_long(FILE *f)
{
	long			out;

	fread((void*)(&out), sizeof(out), 1, f);
	return out;
}

static int				h_digits(int i)
{
	if (i < 10 && i >= 0)
		return 1;
	else if (i < 100 && i >= 0)
		return 2;
	else if (i < 1000 && i >= 0)
		return 3;
	else if (i < 10000 && i >= 0)
		return 4;
	else if (i < 100000 && i >= 0)
		return 5;
	else if (i < 1000000 && i >= 0)
		return 6;
	else if (i < 10000000 && i >= 0)
		return 7;
	else
	{
		std::cerr << "Ridiculous frame count achieved.\n";
		grav_exit();
		return (0);
	}
}

static inline unsigned char	h_avg(unsigned char a, unsigned char b)
{
	int						out;

	out = (a + b) / 2;
	return (unsigned char)out;
}

static inline void		h_put_pixel(int off, Color *c, t_thread *t)
{
	Color				*there;

	if (t->pixels[off + 3])
	{
		check_error(there = Color::get_color_from_table(
			t->pixels[off + 2],
			t->pixels[off + 1],
			t->pixels[off + 0]), "Color there not in table\n");
		there = Color::table[(Color::get_index(c) + Color:: get_index(there)) / 2];
		t->pixels[off + 0] = there->b;
		t->pixels[off + 1] = there->g;
		t->pixels[off + 2] = there->r;
		t->pixels[off + 3] = SDL_ALPHA_OPAQUE;
	}
	else
	{
		t->pixels[off + 0] = c->b;
		t->pixels[off + 1] = c->g;
		t->pixels[off + 2] = c->r;
		t->pixels[off + 3] = SDL_ALPHA_OPAQUE;
	}
}

static inline double	h_read_double(FILE *f)
{
	float			out;
	size_t			r;

	r = fread((void*)(&out), sizeof(out), 1, f);
	check_error(r == 1, "Error reading from file");
	return (double)out;
}

static inline void		h_set_vars(FILE *f, Vector *vec, double &mass)
{
	char				*buf;
	size_t			r;

	buf = new char[16];
	r = fread((void*)buf, 1, 16, f);
	check_error(r > 0, "Error reading from file");
	vec->x = (double)(*(float*)(&buf[0]));
	vec->y = (double)(*(float*)(&buf[4]));
	vec->z = (double)(*(float*)(&buf[8]));
	mass = (double)(*(float*)(&buf[12]));
}

int						thread_func(void *tmp)
{
	t_thread			*t;
	int					dif;
	Vector				*vec;
	Color				*c;
	double				mass;
	int					x, y, off;
	int					width, height, index;
	double				csize, cdif, ccoef, ccons;
	double				gcoef, gcons;

	t = (t_thread*)tmp;
	dif = (t->dad->width - t->dad->height) / 2;
	width = t->dad->width;
	height = t->dad->height;
	cdif = Color::max_mass - Color::min_mass;
	csize = Color::size;
	ccoef = csize / cdif;
	ccons = -csize * Color::min_mass / cdif;
	vec = new Vector();
	SDL_mutexP(t->dad->mutex);
	SDL_CondWait(t->dad->start_cond, t->dad->mutex);
	SDL_mutexV(t->dad->mutex);
	while (true)
	{
		gcoef = (double)width / (t->dad->scale * 3.0);
		gcons = (double)width * (t->dad->scale * 1.5) / (t->dad->scale * 3.0);
		for (long i = 0; i < t->count; i++)
		{
			if (t->dad->excl != 1 && i % t->dad->excl != 0)
			{
				fseek(t->f, 16, SEEK_CUR);
				continue;
			}
			h_set_vars(t->f, vec, mass);
			index = mass * ccoef + ccons;
			if (index >= csize)
				index = csize - 1;
			else if (index < 0)
				index = 0;
			c = Color::table[index];
			t->dad->cam->watch_vector(vec);
			x = gcoef * vec->x + gcons;
			y = gcoef * vec->y + gcons - dif;
			if (x >= 0 && x < width && y >= 0 && y < height)
			{
				off = (t->dad->width * 4 * y) + x * 4;
				h_put_pixel(off, c, t);
			}
		}
		fclose(t->f);
		SDL_mutexP(t->dad->mutex);
		t->dad->running_threads--;
		if (t->dad->running_threads == 0)
			SDL_CondSignal(t->dad->done_cond);
		SDL_CondWait(t->dad->start_cond, t->dad->mutex);
		SDL_mutexV(t->dad->mutex);
	}
	delete vec;
	return 0;
}

void					Render::init_threading()
{
	std::string			name;

	this->num_threads = 4;
	this->mutex = SDL_CreateMutex();
	this->start_cond = SDL_CreateCond();
	this->done_cond = SDL_CreateCond();
	this->threads = std::vector<t_thread*>(this->num_threads);
	this->tex = SDL_CreateTexture(this->ren,
		SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
		this->width, this->height);
	this->pixels = std::vector<unsigned char>(this->width * this->height * 4,
		0);
	for (int i = 0; i < this->num_threads; i++)
	{
		this->threads[i] = new t_thread;
		this->threads[i]->id = i;
		this->threads[i]->dad = this;
		name = "thread_id" + std::to_string(i);
		this->threads[i]->pixels = std::vector<unsigned char>(
			this->width * this->height * 4, 0);
		SDL_CreateThread(thread_func, name.c_str(),
			(void*)this->threads[i]);
	}
}

void					Render::organize_threads(long par)
{
	std::string			name;
	long				cur;
	long				each;
	long				mod;

	cur = 8;
	each = par / this->num_threads;
	mod = par % this->num_threads;
	this->running_threads = this->num_threads;
	std::fill(this->pixels.begin(), this->pixels.end(), 0);
	for (int i = 0; i < this->num_threads; i++)
	{
		name = this->path + std::to_string(this->tick) + ".jgrav";
		this->threads[i]->f = fopen(name.c_str(), "r");
		std::fill(this->threads[i]->pixels.begin(), this->threads[i]->pixels.end(), 0);
		fseek(this->threads[i]->f, cur, SEEK_SET);
		if (i + 1 == this->num_threads)
			each += mod;
		this->threads[i]->count = each;
		cur += each * 16;
	}
}

void					Render::draw(bool first)
{
	FILE				*f;
	std::string			name;
	long				par;
	bool				a, b, c, d;

	name = this->path + std::to_string(this->tick) + ".jgrav";
	f = fopen(name.c_str(), "rb");
	SDL_SetRenderDrawColor(this->ren, 0, 0, 0, 0);
	SDL_RenderClear(this->ren);
	par = h_read_long(f);
	// if (first)
	// 	this->scale = 1000.0 * (double)h_read_long(f);
	organize_threads(par);
	fclose(f);
	SDL_mutexP(this->mutex);
	SDL_CondBroadcast(this->start_cond);
	SDL_CondWait(this->done_cond, this->mutex);
	SDL_mutexV(this->mutex);
	a = false;
	b = false;
	c = false;
	d = false;
	for (int off = 0; off < this->pixels.size(); off++)
	{
		if (off % 4 == 0)
		{
			a = (this->threads[0]->pixels[off + 0] == 0 &&
				this->threads[0]->pixels[off + 1] == 0 &&
				this->threads[0]->pixels[off + 2] == 0 &&
				this->threads[0]->pixels[off + 3] == 0);
			b = (this->threads[1]->pixels[off + 0] == 0 &&
				this->threads[1]->pixels[off + 1] == 0 &&
				this->threads[1]->pixels[off + 2] == 0 &&
				this->threads[1]->pixels[off + 3] == 0);
			c = (this->threads[2]->pixels[off + 0] == 0 &&
				this->threads[2]->pixels[off + 1] == 0 &&
				this->threads[2]->pixels[off + 2] == 0 &&
				this->threads[2]->pixels[off + 3] == 0);
			d = (this->threads[3]->pixels[off + 0] == 0 &&
				this->threads[3]->pixels[off + 1] == 0 &&
				this->threads[3]->pixels[off + 2] == 0 &&
				this->threads[3]->pixels[off + 3] == 0);
		}
		if (!a || !b || !c || !d)
		{
			this->pixels[off] = (
					(a ? 0 : (int)this->threads[0]->pixels[off]) +
					(b ? 0 : (int)this->threads[1]->pixels[off]) +
					(c ? 0 : (int)this->threads[2]->pixels[off]) +
					(d ? 0 : (int)this->threads[3]->pixels[off])
				) / (4 - ((int)a + (int)b + (int)c + (int)d));
		}
	}
	SDL_UpdateTexture(this->tex, NULL, &this->pixels[0], this->width * 4);
	SDL_RenderCopy(this->ren, this->tex, NULL, NULL);
	SDL_RenderPresent(this->ren);
}

static void				h_update_from_input(Render *r, bool *u)
{
	bool				update;

	update = false;
	if (r->keystate[SDL_SCANCODE_W] && (update = true) == true)
		r->cam->mod_angles(M_PI / 32.0, 0.0, 0.0);
	if (r->keystate[SDL_SCANCODE_S] && (update = true) == true)
		r->cam->mod_angles(-M_PI / 32.0, 0.0, 0.0);
	if (r->keystate[SDL_SCANCODE_A] && (update = true) == true)
		r->cam->mod_angles(0.0, -M_PI / 32.0, 0.0);
	if (r->keystate[SDL_SCANCODE_D] && (update = true) == true)
		r->cam->mod_angles(0.0, M_PI / 32.0, 0.0);
	if (r->keystate[SDL_SCANCODE_Q] && (update = true) == true)
		r->cam->mod_angles(0.0, 0.0, -M_PI / 32.0);
	if (r->keystate[SDL_SCANCODE_E] && (update = true) == true)
		r->cam->mod_angles(0.0, 0.0, M_PI / 32.0);
	if (r->keystate[SDL_SCANCODE_UP] && (update = true) == true)
		r->cam->mod_location(0.0, -r->scale / 200.0, 0.0);
	if (r->keystate[SDL_SCANCODE_DOWN] && (update = true) == true)
		r->cam->mod_location(0.0, r->scale / 200.0, 0.0);
	if (r->keystate[SDL_SCANCODE_LEFT] && (update = true) == true)
		r->cam->mod_location(-r->scale / 200.0, 0.0, 0.0);
	if (r->keystate[SDL_SCANCODE_RIGHT] && (update = true) == true)
		r->cam->mod_location(r->scale / 200.0, 0.0, 0.0);
	if (update)
		*u = 0;
}

void					Render::loop(long start, long end)
{
	bool				running;
	bool				paused;
	bool				updated;
	bool				first;
	int					dir;
	SDL_Event			event;

	this->tick = start;
	running = 1;
	paused = 1;
	updated = 0;
	dir = 1;
	first = true;
	while (running)
	{
		if (!paused)
		{
			this->tick += dir;
			updated = 0;
		}
		if (this->tick == end || this->tick == 0)
			paused = 1;
		if (this->tick < start)
		{
			updated = 0;
			this->tick = start;
		}
		else if (this->tick > end)
		{
			updated = 0;
			this->tick = end;
		}
		if (!updated)
		{
			if (first)
			{
				this->draw(true);
				first = false;
			}
			else
				this->draw();
			updated = 1;
		}
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					running = 0;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							running = 0;
							break;
						case SDLK_SPACE:
							paused = !paused;
							break;
						case SDLK_b:
							if (this->tick > start)
								this->tick--;
							updated = 0;
							break;
						case SDLK_n:
							if (this->tick < end)
								this->tick++;
							updated = 0;
							break;
						case SDLK_m:
							this->tick = start;
							updated = 0;
							break;
						case SDLK_r:
							dir *= -1;
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.y < 0)
						this->scale *= 1.1;
					else if (event.wheel.y > 0)
						this->scale /= 1.1;
					updated = 0;
					break;
			}
		}
		this->keystate = (Uint8*)SDL_GetKeyboardState(NULL);
		h_update_from_input(this, &updated);
	}
}

void					Render::wait_for_death()
{
	SDL_Event			e;

	while (1)
		while (SDL_PollEvent(&e))
			switch (e.type)
			{
				case SDL_KEYDOWN:
				case SDL_QUIT:
					grav_exit();
					break;
			}
}

Render::~Render()
{
}
