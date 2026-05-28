# Release Process

This describes the steps for making a release.

## Create a Release PR

* Create a branch that will contain changes for the release
* Update the [CHANGELOG.md](../CHANGELOG.md) using the most recent version as a guide
    * Go through the PRs since the last release and add each PR and descriptive text to the `Breaking changes`, `Features`, `Bug fixes` or `Build changes` sections
* Update the versions in the 3 main CMakeLists.txt files in the `project` blocks, where `VERSION` has the form `<major version>.<minor version>`
    * The files are [bmx CMakeLists.txt](../CMakeLists.txt), [libMXF CmakeLists.txt](../deps/libMXF/CMakeLists.txt) and [libMXFpp CmakeLists.txt](../deps/libMXFpp/CMakeLists.txt)
* Run the [Build & Test](https://github.com/ebu/bmx/actions/workflows/build_and_test.yml) workflow in GitHub Actions using the release branch and fix any build errors and warnings
* Check the [runner versions](https://docs.github.com/en/actions/using-github-hosted-runners/using-github-hosted-runners/about-github-hosted-runners) (e.g. `windows-2019` and `macos-13`) in the [release.yml](../.github/workflows/release.yml) workflow file are still available
    * Select the oldest macOS version available to help with compatibility
* Run the [Release](https://github.com/ebu/bmx/actions/workflows/release.yml) workflow in GitHub Actions using the release branch to check it succeeds
    * On the release branch, create a temporary tag and then delete the tag once the workflow succeeds:

```bash
export BMX_VERSION=<major version>.<minor version>
git tag -a v${BMX_VERSION} -m "Version ${BMX_VERSION}"
git push origin v${BMX_VERSION}
```

```bash
git push --delete origin v${BMX_VERSION}
git tag -d v${BMX_VERSION}
```

* Create and merge a PR for the release branch into `main`

## Create a Release Tag

* Checkout and fetch the `main` branch
* Create a tag with form `v<major version>.<minor version>`. E.g. run the commands below, replacing `<major version>.<minor version>` (with no `v`)

```bash
export BMX_VERSION=<major version>.<minor version>
git checkout main
git pull --rebase
git tag -a v${BMX_VERSION} -m "Version ${BMX_VERSION}"
git push origin v${BMX_VERSION}
```

## Create the Release Packages

* Run the [Release](https://github.com/ebu/bmx/actions/workflows/release.yml) workflow in GitHub Actions
* Download the Artifacts and extract the individual source and binary zips for the release

## Create a GitHub Release

* Create a [new release](https://github.com/ebu/bmx/releases)
* Copy the previous release's text as a starting point
    * Select the `v<major version>.<minor version>` tag
    * Change the CHANGELOG link
    * Update the zip filenames with the new version
    * Update the compiler versions used for the binaries
        * These can be found in the actions output in the `Win64 binary release` and `MacOS Universal binary release` build steps in the 2 jobs of the [Release](https://github.com/ebu/bmx/actions/workflows/release.yml) workflow in GitHub Actions
* Upload the source and binary zips to the release

## Create a Docker Image for the GitHub Container Registry

* Run the [Build and Publish Image](https://github.com/scenarnick/bmx/actions/workflows/publish-image.yml) workflow in GitHub actions on the release tag.
