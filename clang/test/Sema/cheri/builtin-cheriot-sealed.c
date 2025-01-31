// RUN: %clang_cc1 -triple riscv32cheriot -target-abi cheriot-baremetal -verify %s

void test(int * __sealed_capability sealed, int *unsealed) {
	long num = 42;
	(void)__builtin_cheri_tag_get(sealed);
	(void)__builtin_cheri_tag_get(unsealed);
	(void)__builtin_cheri_tag_clear(sealed);
	(void)__builtin_cheri_tag_clear(unsealed);
	(void)__builtin_cheri_address_get(unsealed);
	(void)__builtin_cheri_address_get(sealed);
	(void)__builtin_cheri_address_set(unsealed, num);
	(void)__builtin_cheri_address_set(sealed, num); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_offset_increment(unsealed, num);
	(void)__builtin_cheri_offset_increment(sealed, num); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_perms_get(unsealed);
	(void)__builtin_cheri_perms_get(sealed);
	(void)__builtin_cheri_perms_and(unsealed, num);
	(void)__builtin_cheri_perms_and(sealed, num); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_length_get(unsealed);
	(void)__builtin_cheri_length_get(sealed);
	(void)__builtin_cheri_bounds_set(unsealed, num);
	(void)__builtin_cheri_bounds_set(sealed, num); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_bounds_set_exact(unsealed, num);
	(void)__builtin_cheri_bounds_set_exact(sealed, num); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_base_get(sealed);
	(void)__builtin_cheri_base_get(unsealed);
	(void)__builtin_cheri_type_get(sealed);
	(void)__builtin_cheri_type_get(unsealed);

	(void)__builtin_cheri_seal(sealed, unsealed); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_seal(unsealed, unsealed);
	(void)__builtin_cheri_seal(sealed, sealed); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_seal(unsealed, sealed); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_unseal(sealed, unsealed);
	(void)__builtin_cheri_unseal(unsealed, unsealed); // expected-error{{operand of type 'int *' where sealed capability is required}}
	(void)__builtin_cheri_unseal(sealed, sealed); // expected-error{{operand of type 'int * __sealed_capability' where unsealed capability is required}}
	(void)__builtin_cheri_unseal(unsealed, sealed); // expected-error{{operand of type 'int *' where sealed capability is required}}
}

