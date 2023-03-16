# Contributing

Any and all contributions are welcome. This file is a guide for contributing to the project.

## Issues

If you find a bug, please report it by [creating a new issue](https://github.com/SFMDX/directorscut-swarm/issues) and including the following information:

- The version of the software you are using.
- Your OS and version.
  - For Linux, please include the version of Wine you are using.
- A detailed description of the bug.
- Screenshots or videos of the bug, if possible.
- A list of steps to reproduce the bug.

Issues must be written in English and tagged appropriately.

## Feature Requests

If you have an idea for a new feature, please [create a new issue](https://github.com/SFMDX/directorscut-swarm/issues) with the `enhancement` tag and include the following information:

- A detailed description of the feature.
- How the feature would improve user experience.
- Implementation details, if possible.
- Any mockups or screenshots of the feature, if possible.

Feature requests must be written in English.

## Pull Requests

If you would like to contribute code to the project, please [create a new pull request](https://github.com/SFMDX/directorscut-swarm/pulls) and include the following information:

- A detailed description of the changes.
- A list of steps to test the changes.
- Screenshots or videos of the changes, if possible.
- Why the changes are necessary.
- Specify any license restrictions on the code, if applicable.
- Provide libraries or other files that are required for the changes, if applicable.

Pull requests must be written in English and conform to the [Style Guide](#style-guide).

## Style Guide

The following style guide is used for all code in this project:

- Code that modifies base engine code (Alien Swarm SDK) must be commented with the following format:

```c++
    // DXCHANGE: <Description>
```

- Code that introduces breaking changes must be commented with the following format:

```c++
    // DXFIXME: <Description>
```

- Code specifying future implementation must be commented with the following format:

```c++
    // DXTODO: <Description>
```

- Indentation must be done with tabs with a width of 4.
- Using multiple files is preferred over using a single file.
- Extensive comments are recommended when necessary.
- Variables, methods, classes, and other identifiers must be written in English with the following format:

```c++
    // Classes are written in PascalCase
    class ClassName
    {
        // Methods are also written in PascalCase
        void MethodName();
    };

    // Variables are written in camelCase
    // Any format is acceptable, such as:
    char* stringName;
    int variable_name;
    float fValue;
    bool working;
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for more information.
