#pragma once

template<typename T>
struct LookasideList {
	void Init(POOL_TYPE pool) {
		ExInitializeLookasideListEx();
	}

private:
	LOOKASIDE_LIST_EX m_lookaside;
};
