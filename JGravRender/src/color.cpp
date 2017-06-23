
#include <engine.h>
#include <string>

std::vector<Color*>	Color::table = (std::vector<Color*>)0;
double				Color::min_mass = 0;
double				Color::max_mass = 0;
int					Color::size = 0;

static void			grav_exit()
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
static inline void	check_error(ttype check, std::string message)
{
	if (check == (ttype)0)
	{
		std::cerr << message;
		grav_exit();
	}
}

Color::Color()
{
}

Color::Color(int index)
{
	check_error(index < Color::table.size(),
		"Error: color index out of bounds\n");
	this->r = Color::table[index]->r;
	this->g = Color::table[index]->g;
	this->b = Color::table[index]->b;
}

Color::Color(double mass)
{
	int				index;
	Color			*entry;

	index = (int)((Color::size * (mass - Color::min_mass))
			/ (Color::max_mass - Color::min_mass));
	if (index >= Color::size)
		index = Color::size - 1;
	entry = Color::table[index];
	this->r = entry->r;
	this->g = entry->g;
	this->b = entry->b;
}

Color::Color(unsigned char nr, unsigned char ng, unsigned char nb)
{
	this->r = nr;
	this->g = ng;
	this->b = nb;
}

inline int			Color::get_int()
{
	return (this->r << 16) | (this->g << 8) | this->b;
}

void				Color::init_color_table(t_ctables type, int num)
{
	float			f;
	unsigned char	r;
	unsigned char	g;
	unsigned char	b;

	check_error(num < 1000, "Error: color table too massive\n");
	Color::size = num;
	Color::table = std::vector<Color*>(num);
	if (type == RAINBOW)
	{
		f = 0;
		for (int i = 0; i < num; ++i)
		{
			r = (-cos(f) + 1.0) * 127.0;
			g = (sin(f) + 1.0) * 127.0;
			b = (cos(f) + 1.0) * 127.0;
			Color::table[i] = new Color(r, g, b);
			f += (float)M_PI / (float)num;
		}
	}
}

void				Color::set_range(double min, double max)
{
	Color::min_mass = min;
	Color::max_mass = max;
}

Color				*Color::get_color_from_table(unsigned char r,
	unsigned char g, unsigned char b)
{
	Color			*out;

	out = 0;
	for (int i = 0; i < Color::size; i++)
	{
		if (Color::table[i]->r == r && Color::table[i]->g == g &&
			Color::table[i]->b == b)
		{
			out = Color::table[i];
			break;
		}
	}
	return (out);
}

int					Color::get_index(Color *c)
{
	for (int i = 0; i < Color::size; i++)
		if (Color::table[i]->r == c->r && Color::table[i]->g == c->g &&
			Color::table[i]->b == c->b)
			return i;
	return 0;
}

Color::~Color()
{
}
