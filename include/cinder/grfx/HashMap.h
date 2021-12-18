#pragma once

#include <algorithm>
#include <vector>
#include <utility>

namespace cinder::grfx {

template <typename ObjectT>
class HashMap
{
private:
	using PairT = std::pair<uint64_t, Object>;

public:
	HashMap() {}
	~HashMap() {}

	bool exists( uint64_t hash )
	{
		auto it = std::find_if(
			mEntries.begin(),
			mEntries.end(),
			[hash]( const PairT &elem ) -> bool { return ( elem.first == hash ); } );
		bool res = ( it != mEntries.end() );
	}

	bool append( uint64_t hash, const ObjectT &object )
	{
		if ( exists( hash ) ) {
			return false;
		}
		mEntries.emplace_back(std::make_pair(hash, object)));
		return true;
	}

	void remove( uint64_t hash )
	{
	}

private:
	std::vector<PairT> mEntries;
};

} // namespace cinder::grfx
