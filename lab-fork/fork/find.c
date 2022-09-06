#define _GNU_SOURCE
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

const int ERROR_ID = -1;
const int OK_ID = 0;
const char *WRONG_ARGS = "Wrong args. Synopsis: ./find [-i] <seq_to_find>\n";
const char *OPTIONAL_FLAG = "-i";
const int MAX_FIND_ARGS = 3;
const int MIN_FIND_ARGS = 2;

int REQUIRES_CASE_INSENSITIVE = 0;
int REQUIRES_CASE_SENSITIVE = 1;

typedef bool (*str_recognizer)(const char *haystack, const char *needle);

bool is_str_contained_case_sensitive(const char *haystack, const char *needle);

bool is_str_contained_case_insensitive(const char *haystack, const char *needle);

bool is_a_dotted_dir_name(char *name);

int check_args(int argc, char *argv[], char str_to_find[]);

void create_subdir_path(char current_path[], char subdir_path[], char *subdir_name);

int find_all_matches_within(DIR *directory,
                            char *current_path,
                            const char *str_to_find,
                            str_recognizer is_recognized);

int
main(int argc, char *argv[])
{
	char str_to_find[NAME_MAX];
	int argscheck_result = check_args(argc, argv, str_to_find);
	if (argscheck_result == ERROR_ID) {
		return ERROR_ID;
	}

	DIR *directory = opendir(".");
	if (!directory) {
		perror("Error en opendir");
		return ERROR_ID;
	}

	char initial_dir_path[PATH_MAX] = "./";
	int searching_result = OK_ID;
	if (argscheck_result == REQUIRES_CASE_INSENSITIVE) {
		searching_result =
		        find_all_matches_within(directory,
		                                initial_dir_path,
		                                str_to_find,
		                                is_str_contained_case_insensitive);
	} else if (argscheck_result == REQUIRES_CASE_SENSITIVE) {
		searching_result =
		        find_all_matches_within(directory,
		                                initial_dir_path,
		                                str_to_find,
		                                is_str_contained_case_sensitive);
	}
	closedir(directory);

	if (searching_result == ERROR_ID) {
		perror("Error durante busqueda");
		return ERROR_ID;
	}


	return OK_ID;
}


bool
is_str_contained_case_sensitive(const char *haystack, const char *needle)
{
	return (strstr(haystack, needle) != NULL);
}

bool
is_str_contained_case_insensitive(const char *haystack, const char *needle)
{
	return (strcasestr(haystack, needle) != NULL);
}


int
check_args(int argc, char *argv[], char str_to_find[])
{
	if (argc < MIN_FIND_ARGS) {
		printf("%s", WRONG_ARGS);
		return ERROR_ID;
	} else if (argc > MAX_FIND_ARGS) {
		printf("%s", WRONG_ARGS);
		return ERROR_ID;
	} else if ((argc == MAX_FIND_ARGS) &&
	           (strcmp(argv[1], OPTIONAL_FLAG) != 0)) {
		printf("%s", WRONG_ARGS);
		return ERROR_ID;
	} else if ((argc == MAX_FIND_ARGS) &&
	           (strcmp(argv[1], OPTIONAL_FLAG) == 0)) {
		if (strlen(argv[2]) >= NAME_MAX) {
			return ERROR_ID;
		}
		strcpy(str_to_find, argv[2]);
		return REQUIRES_CASE_INSENSITIVE;
	} else {
		if (strlen(argv[1]) >= NAME_MAX) {
			return ERROR_ID;
		}
		strcpy(str_to_find, argv[1]);
		return REQUIRES_CASE_SENSITIVE;
	}
}

bool
is_a_dotted_dir_name(char *name)
{
	return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

void
create_subdir_path(char current_path[], char subdir_path[], char *subdir_name)
{
	strcpy(subdir_path, current_path);
	strcat(subdir_path, subdir_name);
	strcat(subdir_path, "/");
}

int
find_all_matches_within(DIR *directory,
                        char *current_path,
                        const char *str_to_find,
                        str_recognizer is_recognized)
{
	struct dirent *dir_entry = readdir(directory);
	int current_fd = dirfd(directory);
	while (dir_entry) {
		if (is_a_dotted_dir_name(dir_entry->d_name)) {
			dir_entry = readdir(directory);
			continue;
		}

		if (is_recognized(dir_entry->d_name, str_to_find)) {
			printf("%s%s\n", current_path, dir_entry->d_name);
		}

		if (dir_entry->d_type == DT_DIR) {
			char subdir_path[PATH_MAX];
			create_subdir_path(current_path,
			                   subdir_path,
			                   dir_entry->d_name);

			int subdirectory_fd =
			        openat(current_fd, dir_entry->d_name, O_DIRECTORY);
			DIR *subdirectory = fdopendir(subdirectory_fd);
			if ((subdirectory_fd == ERROR_ID) ||
			    (subdirectory == NULL)) {
				return ERROR_ID;
			}

			int finding_result =
			        find_all_matches_within(subdirectory,
			                                subdir_path,
			                                str_to_find,
			                                is_recognized);
			closedir(subdirectory);
			if (finding_result == ERROR_ID) {
				return ERROR_ID;
			};
		}
		dir_entry = readdir(directory);
	}
	return OK_ID;
}