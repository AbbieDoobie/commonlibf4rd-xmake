#pragma once

// REL::write_fill and REL::replace_func. CommonLibF4RD provides safe_write, safe_fill,
// write_branch, write_call and write_vfunc, but not these two.
//
// Not shimmed: F4RD's Relocation constructors are (ID, std::ptrdiff_t) and
// (ID, VariantOffset); REL::Offset is not accepted as a second argument. Use
// REL::VariantOffset at those call sites. Standalone REL::Offset is unchanged.

#include <REL/Relocation.h>

namespace REL
{
	// Fill a_count bytes at a_dst with a_value.
	inline void write_fill(std::uintptr_t a_dst, std::uint8_t a_value, std::size_t a_count)
	{
		safe_fill(a_dst, a_value, a_count);
	}

	// Overwrite a whole function with a branch to a_dst, then pad the remainder with
	// INT3 so a fallthrough traps loudly instead of executing whatever the original tail
	// happened to be.
	//
	// Requires a trampoline: call F4SE::AllocTrampoline() during plugin load first.
	inline void replace_func(std::uintptr_t a_src, std::size_t a_count, std::uintptr_t a_dst)
	{
		F4SE::GetTrampoline().write_branch<5>(a_src, a_dst);
		if (a_count > 5) {
			safe_fill(a_src + 5, INT3, a_count - 5);
		}
	}

	template <class F>
	inline void replace_func(std::uintptr_t a_src, std::size_t a_count, F a_dst)
	{
		replace_func(a_src, a_count, F4SE::stl::unrestricted_cast<std::uintptr_t>(a_dst));
	}

	// REL::write_call<N, O>(rel, dst). F4RD has write_call only on Trampoline, so the
	// relocation is the first argument:
	//     REL::write_call<5, 0x40>(rel, &Hook)
	template <std::size_t N, std::ptrdiff_t O = 0, class F>
	std::uintptr_t write_call(const Relocation<std::uintptr_t>& a_rel, F a_dst)
	{
		return F4SE::GetTrampoline().write_call<N>(
			a_rel.address() + O,
			F4SE::stl::unrestricted_cast<std::uintptr_t>(a_dst));
	}
}
