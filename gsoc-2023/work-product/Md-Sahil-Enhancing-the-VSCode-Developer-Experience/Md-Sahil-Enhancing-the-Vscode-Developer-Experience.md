# Unikraft GSoC'23: Enhancing the VSCode Developer Experience

### Description
The VS Code Extension for Unikraft enables developers to quickly and painlessly build unikernels from the VS Code IDE.
Amongst other features, it allows developers to list and find unikernel libraries as well as run basic commands.
In this project, I upgraded the VS Code extension to use [KraftKit](https://github.com/unikraft/kraftkit),
the newly released CLI companion tool for Unikraft, written in Go as well as created a few commands for kraftkit.

### Preview (Unikraft's vs-code IDE extension usage & working)

Listing packages available on the system & project directory:

[listing_packages.webm](https://user-images.githubusercontent.com/85174511/262644232-96730487-ac27-4ab1-820f-d038f76a22cf.webm)

Adding a package to the project:

[adding_package.webm](https://user-images.githubusercontent.com/85174511/262644505-a2ba7f3a-961e-4c8c-9ad6-d2fa6a276c65.webm)

Removing and purging a pakcage:

[removing&purging_package.webm](https://user-images.githubusercontent.com/85174511/262644742-3a9a4f42-c40a-405f-8670-d34299d63466.webm)

Fetching dependencies:

[fetching.webm](https://user-images.githubusercontent.com/85174511/262645570-5c2ad777-329f-49ae-a892-5848983e8202.webm)

Preparing a project to run:

[preparing.webm](https://user-images.githubusercontent.com/85174511/262646044-ceb1fc9a-33b7-48c3-bc1c-669db6633c9f.webm)

Building a project to run:

[building.webm](https://user-images.githubusercontent.com/85174511/262646200-5901675b-3c25-40a9-8030-6a76254f594e.webm)

Running a project:

[running.webm](https://user-images.githubusercontent.com/85174511/262646373-50c39020-f5ff-410f-a8e0-ded1b9856f5c.webm)

Initialising a library:

[creating_library.webm](https://user-images.githubusercontent.com/85174511/262646570-0c206a8d-a024-48e9-95ba-3f72786c296a.webm)

Cleaning a built project:

[cleaning.webm](https://user-images.githubusercontent.com/85174511/262646945-10c8238c-95c3-48ed-8c9d-a5287bf13fae.webm)

Cleaning builts properly:

[proper_cleaning.webm](https://user-images.githubusercontent.com/85174511/262647105-f3d56682-6414-4722-abda-11441b39d39a.webm)

## GSoC contributor
Name: Md Sahil

Email: contact.afsaransari@gmail.com

Github profile: https://github.com/MdSahil-oss

## Contributions
For this project, I made enhancement in two diffrent repositories [Kraftkit](https://github.com/unikraft/kraftkit) and [ide-vscode](https://github.com/unikraft/ide-vscode) of Unikraft. Where Kraftkit repository contains all the enhancements related to CLI commands development and ide-vscode repository contains all enhancements have been made in Unikraft Vscode IDE extension to make it compatible with `Kraftkit` CLI.

### PR link in [ide-vscode repository](https://github.com/unikraft/ide-vscode)
 - [Update VS Code IDE extension to function properly with new Go-based KraftKit](https://github.com/unikraft/ide-vscode/pull/7)
### PR links in [kraftkit repository](https://github.com/unikraft/kraftkit/)
 - [Configurable output format](https://github.com/unikraft/kraftkit/pull/557/files)
 - [Added a subcommand `show` for command `kraft pkg`](https://github.com/unikraft/kraftkit/pull/536)
 - [Adds `kraft lib create` support](https://github.com/unikraft/kraftkit/pull/591)
 - [Add new subcommand `kraft pkg add`](https://github.com/unikraft/kraftkit/pull/622)
 - [Add new subcommand `kraft pkg rm`](https://github.com/unikraft/kraftkit/pull/631)
 - [Add subcommand `prune` for the command `kraft pkg`](https://github.com/unikraft/kraftkit/pull/663)

## Blog posts
 - [First blog post](https://github.com/unikraft/docs/pull/269)
 - [Second blog post](https://github.com/unikraft/docs/pull/293)
 - [Third blog post](https://github.com/unikraft/docs/pull/303)
 - [Forth blog post](https://github.com/unikraft/docs/pull/305)

## Current status
The main part of my project is almost done (Only pull requests are pending to be reviewed & merged by mentors), Now IDE extension is working fine with newly created golang based CLI Kraftkit as well as able to execute some new commands that Kraftkit has like `Fetch`, `Prepare`, `Clean` and `Properclean`.

## Future work
Some additional tasks are left to be accomplished in this project that are:
* Adding support in Unikraft's build cycle for packaging unikernels in different formats.
* The provisioning of an [LSP](https://code.visualstudio.com/api/language-extensions/language-server-extension-guide) that would allow checks when writing Unikraft code such that relevant libraries are imported or not.

Once my created PRs get merged I'll start working on these remaining additional tasks one by one beyond GSoC.
