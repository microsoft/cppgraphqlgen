// This is a dummy program that just needs to compile and link to tell us if
// the C++17 std::filesystem API requires any additional libraries.

#include <filesystem>

int main()
{
	try
	{
		throw std::filesystem::filesystem_error("instantiate one to make sure it links",
			std::make_error_code(std::errc::function_not_supported));
	}
	catch (const std::filesystem::filesystem_error& error)
	{
		return -1;
	}

	return !std::filesystem::temp_directory_path().is_absolute();
}