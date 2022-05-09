#ifndef CSV_H
#define CSV_H

struct CSV_Date
{
  int day;
  int month;
  int year;
};

void csv_setopt(int sem);
void csv_separator(const char *s, const char **end);
char *csv_string(const char *s, const char **end);
double csv_double(const char *s, const char **end);
long csv_long(const char *s, const char **end);

#endif
