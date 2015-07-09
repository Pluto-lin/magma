
/**
 * @file /magma.check/user/user_check.c
 *
 * @brief Checks the code used to handle user data.
 *
 * $Author$
 * $Date$
 * $Revision$
 *
 */

#include "magma_check.h"

START_TEST (check_users_credentials_valid_s) {

	int_t state;
	stringer_t *errmsg = NULL;
	meta_user_t *user_check_data = NULL;
	credential_t *user_check_cred = NULL;

	typedef struct {
		stringer_t *username;
		stringer_t *password;
	} name_pass;

	name_pass tests[3] = {
		{
			CONSTANT("magma"),
			CONSTANT("test")
		},
		{
			CONSTANT("magma@lavabit.com"),
			CONSTANT("test")
		},
		{
			CONSTANT("magma+label@lavabit.com"),
			CONSTANT("test")
		}
	};

	// Valid Login Attempts
	log_unit("%-64.64s", "USERS / CREDENTIAL / VALID / SINGLE THREADED:");

	for(uint_t i = 0; i < (sizeof(tests)/sizeof(tests[0])); ++i) {

		if(!(user_check_cred = credential_alloc_auth(tests[i].username))) {
			errmsg = st_aprint("Credential allocation failed. { user = %s }", st_char_get(tests[i].username));
			goto end;
		}

		if(!credential_calc_auth(user_check_cred, tests[i].password, NULL)) {
			errmsg = st_aprint("Credential allocation failed. { password = %s / salt = NULL }", st_char_get(tests[i].password));
			goto cleanup_cred;
		}

		state = meta_get(user_check_cred, META_PROT_GENERIC, (META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS), &user_check_data);

		if(state != 1) {
			errmsg = st_aprint("Authentication failed. { user = %s / password = %s / salt = NULL / meta_get = %i }",
					st_char_get(tests[i].username), st_char_get(tests[i].password), state);
			goto cleanup_cred;
		}

		meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);

		if(meta_user_prune(user_check_cred->auth.username) < 1) {
			errmsg = st_aprint("An error occurred while trying to prune the user data from the object cache. { user = %s }", st_char_get(tests[i].username));
			goto cleanup_cred;
		}

		credential_free(user_check_cred);
	}

end:
	log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	fail_unless(!errmsg, st_char_get(errmsg));
cleanup_cred:
	credential_free(user_check_cred);
	goto end;

} END_TEST

START_TEST (check_users_credentials_invalid_s) {

	int_t state;
	stringer_t *errmsg = NULL;
	meta_user_t *user_check_data = NULL;
	credential_t *user_check_cred = NULL;
	stringer_t *username = NULL, *password = NULL;

	typedef struct {
		stringer_t *username;
		stringer_t *password;
	} name_pass;

	name_pass tests[4] = {
		{
			CONSTANT("magma"),
			CONSTANT("password")
		},
		{
			CONSTANT("magma@lavabit.com"),
			CONSTANT("password")
		},
		{
			CONSTANT("magma+label@lavabit.com"),
			CONSTANT("password")
		},
		{
			CONSTANT("magma@nerdshack.com"),
			CONSTANT("test")
		}
	};

	// Invalid Login Attempts
	log_unit("%-64.64s", "USERS / CREDENTIAL / INVALID / SINGLE THREADED:");

	// Try passing in various combinations of NULL.
	if ((user_check_cred = credential_alloc_auth(NULL))) {
		errmsg = st_merge("n", "Credential allocation should have failed but succeeded instead. { user = NULL }");
		goto cleanup_cred;
	}

	if (!(user_check_cred = credential_alloc_auth(CONSTANT("magma")))) {
		errmsg = st_merge("n", "Credential allocation failed. { user = magma }");
		goto end;
	}

	for(uint_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {

		if (!(user_check_cred = credential_alloc_auth(tests[i].username))) {
			errmsg = st_aprint("Credential creation failed. Authentication was not attempted. { user = %s }", st_char_get(tests[i].username));
			goto end;
		}

		if(!credential_calc_auth(user_check_cred, tests[i].password, NULL)) {
			errmsg = st_aprint("Credential calculation failed. { password = %s / salt = NULL}", st_char_get(tests[i].password));
			goto cleanup_cred;
		}

		if ((state = meta_get(user_check_cred, META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) == 1) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = %s / password = %s / meta_get = %i }",
					st_char_get(tests[i].username), st_char_get(tests[i].password), state);
			goto cleanup_data;
		}

		credential_free(user_check_cred);
	}


	// Attempt a credentials creation using a series of randomly generated, but valid usernames.
	/* In my opinion these tests are no good so I am commenting them out - IVAN */
/*
	for (uint_t i = 0; !errmsg && i < OBJECT_CHECK_ITERATIONS; i++) {
		if (!(username = rand_choices("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_", (rand_get_uint8() % 128) + 1))) {
			errmsg = st_import("An error occurred while trying to generate a random username. { user = NULL }", 78);
		}
		else if (!(user_check_cred = credential_alloc_auth(username, CONSTANT("test")))) {
			errmsg = st_aprint("Credential creation failed. Authentication was not attempted. { user = %.*s / password = test }",	st_length_int(username), st_char_get(username));
		}
		else if ((state = meta_get(user_check_cred, META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) != 0) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = RANDOM BINARY DATA / password = RANDOM BINARY DATA / meta_get = %i }", state);
		}

		if (user_check_data) {
			meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
			user_check_data = NULL;
		}

		if (user_check_cred) {
			credential_free(user_check_cred);
			user_check_cred = NULL;
		}

		st_cleanup(username);
	}

	// Attempt a credentials creation using a series of randomly generated binary usernames.
	for (uint_t i = 0; !errmsg && i < OBJECT_CHECK_ITERATIONS; i++) {

		username = st_alloc((rand_get_uint8() % 128) + 1);

		if (!username || !rand_write(username)) {
			errmsg = st_import("An error occurred while trying to generate a random username. { user = NULL }", 78);
		}
		else if ((user_check_cred = credential_alloc_auth(username, CONSTANT("test"))) && (state = meta_get(user_check_cred,
				META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) != 0) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = RANDOM BINARY DATA / password = test / meta_get = %i }", state);
		}

		if (user_check_data) {
			meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
			user_check_data = NULL;
		}

		if (user_check_cred) {
			credential_free(user_check_cred);
			user_check_cred = NULL;
		}

		st_cleanup(username);
	}

	// Finally, try generating credentials using randomly generated binary data for the username and the password.
	for (uint_t i = 0; !errmsg && i < OBJECT_CHECK_ITERATIONS; i++) {

		username = st_alloc((rand_get_uint8() % 128) + 1);
		password = st_alloc((rand_get_uint8() % 255) + 1);

		if (!username || !rand_write(username) || !password || !rand_write(password)) {
			errmsg = st_import("Random username and password generation failed. { user = NULL / password = NULL }", 82);
		}
		else if ((user_check_cred = credential_alloc_auth(username, password)) && (state = meta_get(user_check_cred, META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) != 0) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = RANDOM BINARY DATA / password = RANDOM BINARY DATA / meta_get = %i }", state);
		}

		if (user_check_data) {
			meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
			user_check_data = NULL;
		}

		if (user_check_cred) {
			credential_free(user_check_cred);
			user_check_cred = NULL;
		}

		st_cleanup(username);
		st_cleanup(password);
	}
*/

end:
	log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	fail_unless(!errmsg, st_char_get(errmsg));
cleanup_data:
	meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
cleanup_cred:
	credential_free(user_check_cred);
	goto end;

} END_TEST

START_TEST (check_users_inbox_s) {

	char *errmsg = NULL;

	log_unit("%-64.64s", "USERS / INBOX / SINGLE THREADED:");
	errmsg = "User inbox test incomplete.";
	log_unit("%10.10s\n", "SKIPPED");

	//log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	//fail_unless(!errmsg, errmsg);
} END_TEST

START_TEST (check_users_message_s) {

	char *errmsg = NULL;

	log_unit("%-64.64s", "USERS / MESSAGE / SINGLE THREADED:");
	errmsg = "User message test incomplete.";
	log_unit("%10.10s\n", "SKIPPED");

	//log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	//fail_unless(!errmsg, errmsg);
} END_TEST

Suite * suite_check_users(void) {

	TCase *tc;
	Suite *s = suite_create("\tUsers");

	testcase(s, tc, "Auth Valid/S", check_users_credentials_valid_s);
	testcase(s, tc, "Auth Invalid/S", check_users_credentials_invalid_s);
	testcase(s, tc, "Inbox/S", check_users_inbox_s);
	testcase(s, tc, "Message/S", check_users_message_s);

	return s;
}
