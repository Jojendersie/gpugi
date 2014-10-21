#include "pathutils.hpp"
#include "assert.hpp"

namespace PathUtils
{
	/// Concats to paths.
	std::string AppendPath(const std::string& _leftPath, const std::string& _rightPath)
	{
		return CanonicalizePath(_leftPath + "/" + _rightPath);
	}

	std::string CanonicalizePath(const std::string& _path)
	{
		std::string canonicalizedPath(_path);

		size_t searchResult = 0;
		while ((searchResult = canonicalizedPath.find('\\', searchResult)) != std::string::npos)
		{
			canonicalizedPath[searchResult] = '/';
		}

		searchResult = 0;
		while ((searchResult = canonicalizedPath.find("//", searchResult)) != std::string::npos)
		{
			canonicalizedPath.erase(searchResult, 1);
			--searchResult;
		}

		searchResult = 1;
		while ((searchResult = canonicalizedPath.find("..", searchResult)) != std::string::npos)
		{
			if (searchResult > 2 && canonicalizedPath[searchResult - 1] == '/')
			{
				size_t slashBefore = searchResult - 2;
				for (; slashBefore > 0; --slashBefore)
				{
					if (canonicalizedPath[slashBefore-1] == '/')
						break;
				}
				--slashBefore;
				canonicalizedPath.erase(slashBefore, searchResult - slashBefore + 2);
				searchResult -= searchResult - slashBefore;
			}
		}

		return canonicalizedPath;
	}

	size_t GetFilenameStart(const std::string& _path)
	{
		Assert(_path == CanonicalizePath(_path), "Given path was expected to be canonicalized: \'" + _path + "\'");
		return _path.find_last_of('/');
	}


	std::string GetFilename(const std::string& _path)
	{
		Assert(_path == CanonicalizePath(_path), "Given path was expected to be canonicalized: \'" + _path + "\'");
		return _path.substr(_path.find_last_of('/') + 1);
	}

	std::string GetDirectory(const std::string& _path)
	{
		Assert(_path == CanonicalizePath(_path), "Given path was expected to be canonicalized: \'" + _path + "\'");
		return _path.substr(0, _path.find_last_of('/'));
	}

} // PathUtils