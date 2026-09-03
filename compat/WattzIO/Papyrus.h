#pragma once

// Variadic Papyrus dispatch. CommonLibF4RD declares only the raw virtual
// IVirtualMachine::DispatchStaticCall; these add the convenience overload as free
// functions.
//
//     WIO::Papyrus::DispatchStaticCall(vm, obj, func, callback, a, b)
//
// The argument-pack parameter is BSTThreadScrapFunction, which F4RD types as
// msvc::function (32 bytes). It has no constructor - it can only receive a function
// object from the game - and its layout does not match MSVC's std::function, which is
// what the game passes. The parameter is taken by const reference, so only an address
// crosses the call: a std::function is built here and its reference reinterpreted to
// F4RD's type. Nothing constructs or destroys through the mismatched type.

#include <RE/Fallout.h>

#include <functional>
#include <tuple>
#include <utility>

namespace WIO::Papyrus
{
	namespace detail
	{
		template <class... Args>
		[[nodiscard]] RE::BSScrapArray<RE::BSScript::Variable> PackVariables(Args&&... a_args)
		{
			constexpr auto size = sizeof...(a_args);
			auto args = std::make_tuple(std::forward<Args>(a_args)...);
			RE::BSScrapArray<RE::BSScript::Variable> result{ size };
			[&]<std::size_t... p>(std::index_sequence<p...>) {
				((RE::BSScript::PackVariable(result.at(p), std::get<p>(args))), ...);
			}(std::make_index_sequence<size>{});
			return result;
		}
		using ScrapFn = RE::BSTThreadScrapFunction<bool(RE::BSScrapArray<RE::BSScript::Variable>&)>;

		[[nodiscard]] inline const ScrapFn& as_scrap_fn(
			const std::function<bool(RE::BSScrapArray<RE::BSScript::Variable>&)>& a_fn) noexcept
		{
			return reinterpret_cast<const ScrapFn&>(a_fn);
		}
	}

	// a_vm is templated so it accepts both a raw IVirtualMachine* and the
	// BSTSmartPointer<IVirtualMachine> that GameVM::GetVM() hands back.
	template <class VM, class... Args>
	bool DispatchStaticCall(
		VM&& a_vm,
		const RE::BSFixedString& a_objName,
		const RE::BSFixedString& a_funcName,
		const RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>& a_callback,
		Args&&... a_args)
	{
		if (!a_vm) {
			return false;
		}
		const std::function<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> pack =
			[&](RE::BSScrapArray<RE::BSScript::Variable>& a_out) {
				a_out = detail::PackVariables(std::forward<Args>(a_args)...);
				return true;
			};
		return a_vm->DispatchStaticCall(a_objName, a_funcName, detail::as_scrap_fn(pack), a_callback);
	}

	template <class VM, class... Args>
	bool DispatchMethodCall(
		VM&& a_vm,
		std::uint64_t a_objHandle,
		const RE::BSFixedString& a_objName,
		const RE::BSFixedString& a_funcName,
		const RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>& a_callback,
		Args&&... a_args)
	{
		if (!a_vm) {
			return false;
		}
		const std::function<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> pack =
			[&](RE::BSScrapArray<RE::BSScript::Variable>& a_out) {
				a_out = detail::PackVariables(std::forward<Args>(a_args)...);
				return true;
			};
		return a_vm->DispatchMethodCall(a_objHandle, a_objName, a_funcName,
			detail::as_scrap_fn(pack), a_callback);
	}

	// Raw forms, for call sites that build the argument array themselves and pass the
	// callback last - the virtual's own parameter order.
	template <class VM, class Fn>
	bool DispatchStaticCallRaw(
		VM&& a_vm,
		const RE::BSFixedString& a_objName,
		const RE::BSFixedString& a_funcName,
		Fn&& a_pack,
		const RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>& a_callback)
	{
		if (!a_vm) {
			return false;
		}
		const std::function<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> pack =
			std::forward<Fn>(a_pack);
		return a_vm->DispatchStaticCall(a_objName, a_funcName, detail::as_scrap_fn(pack), a_callback);
	}

	template <class VM, class Obj, class Fn>
	bool DispatchMethodCallRaw(
		VM&& a_vm,
		Obj&& a_object,
		const RE::BSFixedString& a_funcName,
		Fn&& a_pack,
		const RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>& a_callback)
	{
		if (!a_vm) {
			return false;
		}
		const std::function<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> pack =
			std::forward<Fn>(a_pack);
		return a_vm->DispatchMethodCall(std::forward<Obj>(a_object), a_funcName,
			detail::as_scrap_fn(pack), a_callback);
	}
}
