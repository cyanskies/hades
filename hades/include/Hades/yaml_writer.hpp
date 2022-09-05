#ifndef HADES_YAML_WRITER_HPP
#define HADES_YAML_WRITER_HPP

#include <memory>

#include "hades/writer.hpp"

namespace hades::data
{
	std::unique_ptr<writer> make_yaml_writer();
	std::unique_ptr<writer> make_yaml_writer(std::ostream&);
}

#endif //!HADES_YAML_WRITER_HPP
