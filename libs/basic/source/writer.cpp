#include "hades/writer.hpp"

#include <cassert>

static hades::data::make_writer_f make_writer_ptr = {};

void hades::data::set_default_writer(make_writer_f f)
{
	make_writer_ptr = f;
}

std::unique_ptr<hades::data::writer> hades::data::make_writer()
{
	assert(make_writer_ptr);
	return std::invoke(make_writer_ptr);
}
