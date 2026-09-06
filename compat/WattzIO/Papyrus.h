#pragma once

// === F4RD RELOCATIONS ========================================================
// kind  what                                                OG      NG/AE
// abi   BSTThreadScrapFunction impl-pointer offset, read
//       by IVirtualMachine's dispatch slots                 0x18    0x38
//
// Not an address: nothing resolves it, REL::Module's family picks it. Listed
// here because a family-wide banner grep has to see everything the compat layer
// pins to a runtime, not only its ids.
//
// Re-derive:
//   1. Resolve RE::VTABLE::BSScript__Internal__VirtualMachine[0] and read slot
//      0x2C, DispatchStaticCall (OG 1.10.163 rva 0x2736470, NG 1.10.984
//      rva 0x1FBF930).
//   2. Its fourth argument, r9, is the functor. Find the `mov rcx, [r9 + N]`
//      whose result is dereferenced and called through `[rax + 0x10]` - vtable
//      slot 2, _Do_call. N is the offset.
//   3. Cross-check SendEvent (slot 0x2B) and the shared worker the
//      DispatchMethodCall overloads forward to (OG 0x2743640). Both read the
//      same N, so one shim covers every dispatch here.
// =============================================================================

// Variadic Papyrus dispatch. CommonLibF4RD declares only the raw virtual
// IVirtualMachine::DispatchStaticCall; these add the convenience overload as free
// functions.
//
//     WIO::Papyrus::DispatchStaticCall(vm, obj, func, callback, a, b)
//
// The argument-pack parameter is BSTThreadScrapFunction, which is the game's own
// std::function. It has no constructor - it can only receive a function object from the
// game - so the object handed to it is built here, by hand, to the layout the running
// runtime expects.
//
// THE LAYOUT IS NOT THE SAME ON EVERY RUNTIME, and it is not our compiler's either. The
// game reads the impl pointer out of that object at a hardcoded offset, in
// VirtualMachine::DispatchStaticCall (vtable slot 0x2C):
//
//     OG 1.10.163  0x2736470   mov rcx, [rbx + 0x18]   VS2015-era STL
//     NG 1.10.984  0x1FBF930   mov rcx, [rbx + 0x38]   modern STL
//
// then does `mov rax, [rcx]` and `call [rax + 0x10]` - slot 2 of the impl's vtable. The
// same offset holds for SendEvent (slot 0x2B) and for the shared worker the
// DispatchMethodCall overloads forward to (OG 0x2743640), so one shim covers every
// dispatch below.
//
// This used to build a real std::function and reinterpret_cast a reference to it. That
// worked on NG/AE by coincidence - MSVC 14.4x puts the impl pointer at 0x38, so the two
// agreed - and crashed on OG, where the game read offset 0x18 and found captured lambda
// payload, dereferenced it, and called through it. Only the mods that reach MCM through
// Papyrus were affected, which is what made the split look like an MCM problem.
//
// So nothing below constructs a std::function. Proxy is our own object with do_call in
// slot 2, and ScrapFnShim places a pointer to it at the offset this runtime reads.

#include <RE/Fallout.h>

#include <cstddef>
#include <cstdint>
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
		using PackArgs = RE::BSScrapArray<RE::BSScript::Variable>&;

		// std::_Func_base, as far as the game uses it. Only slot 2 is ever reached - the
		// functor is a const& parameter, so the callee neither copies nor destroys it - but
		// the earlier slots have to exist to put do_call at index 2.
		class __declspec(novtable) ProxyBase
		{
		public:
			virtual ProxyBase*  copy(void*) = 0;           // 00
			virtual ProxyBase*  move(void*) = 0;           // 01
			virtual bool        do_call(PackArgs) = 0;     // 02
			virtual const void* target_type() const = 0;   // 03
			virtual void        delete_this(bool) = 0;     // 04
		};

		// Holds the packing callable by reference: the shim never outlives the call, and
		// the callable is a local in the same scope.
		template <class F>
		class Proxy final : public ProxyBase
		{
		public:
			explicit Proxy(F& a_fn) noexcept :
				_fn(a_fn)
			{}

			ProxyBase*  copy(void*) override { return this; }
			ProxyBase*  move(void*) override { return this; }
			bool        do_call(PackArgs a_out) override { return _fn(a_out); }
			const void* target_type() const override { return nullptr; }
			void        delete_this(bool) override {}

		private:
			F& _fn;
		};

		// The object the game receives. 64 bytes so the NG/AE slot is inside it; the
		// declared ScrapFn is smaller, but only a reference to this ever crosses the call.
		template <class F>
		class ScrapFnShim
		{
		public:
			explicit ScrapFnShim(F& a_fn) noexcept :
				_proxy(a_fn)
			{
				*reinterpret_cast<ProxyBase**>(_storage + ImplOffset()) = &_proxy;
			}

			ScrapFnShim(const ScrapFnShim&) = delete;
			ScrapFnShim& operator=(const ScrapFnShim&) = delete;

			[[nodiscard]] const ScrapFn& get() const noexcept
			{
				return reinterpret_cast<const ScrapFn&>(_storage);
			}

		private:
			[[nodiscard]] static std::size_t ImplOffset() noexcept
			{
				return REL::Module::get().is_og() ? 0x18 : 0x38;  // F4RD:abi
			}

			static_assert(0x38 + sizeof(void*) <= 64, "storage must cover every runtime's impl offset");

			alignas(16) std::byte _storage[64]{};
			Proxy<F> _proxy;
		};

		template <class F>
		ScrapFnShim(F&) -> ScrapFnShim<F>;
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
		auto pack = [&](RE::BSScrapArray<RE::BSScript::Variable>& a_out) {
			a_out = detail::PackVariables(std::forward<Args>(a_args)...);
			return true;
		};
		detail::ScrapFnShim shim{ pack };
		return a_vm->DispatchStaticCall(a_objName, a_funcName, shim.get(), a_callback);
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
		auto pack = [&](RE::BSScrapArray<RE::BSScript::Variable>& a_out) {
			a_out = detail::PackVariables(std::forward<Args>(a_args)...);
			return true;
		};
		detail::ScrapFnShim shim{ pack };
		return a_vm->DispatchMethodCall(a_objHandle, a_objName, a_funcName, shim.get(), a_callback);
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
		auto&& pack = std::forward<Fn>(a_pack);
		detail::ScrapFnShim shim{ pack };
		return a_vm->DispatchStaticCall(a_objName, a_funcName, shim.get(), a_callback);
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
		auto&& pack = std::forward<Fn>(a_pack);
		detail::ScrapFnShim shim{ pack };
		return a_vm->DispatchMethodCall(std::forward<Obj>(a_object), a_funcName, shim.get(), a_callback);
	}
}
