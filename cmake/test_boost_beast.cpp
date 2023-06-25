// This is a dummy program that just needs to compile to tell us if Boost.Asio
// supports co_await and Boost.Beast is installed.

#include <boost/asio/use_awaitable.hpp>

#include <boost/beast.hpp>

int main()
{
#ifdef BOOST_ASIO_HAS_CO_AWAIT
	return 0;
#else
	#error BOOST_ASIO_HAS_CO_AWAIT is undefined
#endif
}