
#ifndef COLOR_H
# define COLOR_H

# include "engine.h"

typedef enum					e_ctables
{
	RAINBOW
}								t_ctables;

class							Color
{
public:
	unsigned char				r;
	unsigned char				g;
	unsigned char				b;
	static std::vector<Color*>	table;
	static double				min_mass;
	static double				max_mass;
	static int					size;

	Color();
	Color(int index);
	Color(double mass);
	Color(unsigned char nr, unsigned char ng, unsigned char nb);
	inline int					get_int();
	static void					init_color_table(t_ctables type, int num);
	static void					set_range(double min, double max);
	static Color				*get_color_from_table(unsigned char r,
		unsigned char g, unsigned char b);
	static int					get_index(Color *c);
	~Color();
};

#endif
