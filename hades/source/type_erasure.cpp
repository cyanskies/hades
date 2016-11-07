#include "Hades/type_erasure.hpp"

namespace hades {
	namespace type_erasure {
		typename type_erased_base::size_type type_count = 0;

		template<class T>
		typename type_erased::size_type type_erased_base::_typeId = type_count++;
	}
}