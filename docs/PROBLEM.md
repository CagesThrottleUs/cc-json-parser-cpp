# Problem Statement

We have to create a simple CLI application that will validate the JSON provided by the user. The JSON is described with
the attached document. (Language Specification Document) [here](./language_document.pdf)

The CLI application must accept file path as argument.

# Where to get tests

I got three set of challenges that I have put into three different directories.

1. [challenges](https://www.dropbox.com/s/vthtr4897fkuhw8/tests.zip?dl=1)
2. [testsuite](https://www.json.org/JSON_checker/test.zip)
3. [hardcore](https://download-directory.github.io/?url=https%3A%2F%2Fgithub.com%2Fnst%2FJSONTestSuite%2Ftree%2Fmaster%2Ftest_parsing)

# How will program communicate with the world

The CLI program will communicate with the caller by providing the following two information:

1. Exit Code as 0 if the JSON is valid, Exit Code as 1 if the JSON is invalid. Other reserved exit codes can be used to
   indicate other processing errors.
2. It will display important information about failures from the such as unexpected keyword found etc. (lexical failures)
3. Also display failure information about the parsing error.

# Testing mechanism

We will be testing the program generated as shown in the [README.md](../README.md).