## How to Contribute

We use the standard **fork and pull request** workflow. This applies to
everyone, including maintainers — all changes go through a fork and a pull
request, so there are no long-lived branches on the main repository.

These conventions aren't set in stone — the goal is that all changes go through
review and the main repository stays uncluttered.


1. **Fork** the [repository](https://github.com/ebu/bmx) to your own GitHub account.
   Note that keeping your forked projects name "bmx" comes with benefits
   for example when testing the Release Workflow. Likewise, enabling GitHub
   Actions on your fork lets you run the workflows against your own changes.
2. **Clone** your fork and create a topic branch for your work:
   ```bash
   git clone --recursive https://github.com/<your-username>/bmx.git
   cd bmx
   git checkout -b my-change
   ```
   Note the `--recursive` flag — bmx includes the `libMXF` and `libMXF++`
   dependencies as submodules in `deps/`.
3. **Add the upstream repository** as a remote (once) so you can pull in the
   latest changes:
   ```bash
   git remote add upstream https://github.com/ebu/bmx.git
   ```
   Keep your branch up-to-date with the main repo:
   ```bash
   git pull upstream main
   ```
   
4. **Make your changes** on the topic branch, committing in logical steps with
   clear commit messages.
5. **Run the tests** to make sure everything still passes, and add tests for any
   new behaviour — see the [Tests](#tests) section.
6. **Push** the branch to your fork and **open a pull request** against the
   `main` branch of `ebu/bmx`. Opening the pull request is normally done from
   the GitHub UI.

Please describe *what* the change does and *why* in the pull request. If it
fixes an issue, reference it (e.g. `Fixes #123`).

## Tests

Good test coverage is what lets the community keep this project healthy and
refactor with confidence.

- **New features should come with corresponding tests.** A pull request that
  adds or changes behaviour is expected to include tests that exercise it.
- **Bug fixes** should ideally add a regression test that fails before the fix
  and passes after it.
- All existing tests must continue to pass.

### Building and running the tests

See [docs/build.md](docs/build.md) for how to configure, build and run the tests.

Tests live under [`test/`](test/), grouped by feature (e.g. `mxf_op1a`, `imf`,
`avid_mxf`). When adding a feature test, follow the structure of the existing
tests in that directory and register it via the project's CMake test
machinery (see [`test/testing.cmake`](test/testing.cmake)).

### Continuous integration

Every pull request is automatically built and tested on Linux, macOS and
Windows via GitHub Actions (see
[`.github/workflows/build_and_test.yml`](.github/workflows/build_and_test.yml)),
including a Valgrind run on Linux. Please make sure your branch builds cleanly
and the tests pass before requesting review.

### Changing the build system

Changes to the build system (the `CMakeLists.txt` files, `cmake/` modules, etc.)
deserve extra care, since they affect everyone building bmx:

- Build and test **both `Release` and `Debug`** configurations locally, not just
  one, as configuration-specific issues are easy to miss.
- Also exercise the **[Release](.github/workflows/release.yml) workflow** to make
  sure packaging still succeeds. Running it on your fork is the easiest way to do
  this — see the steps under "Run the Release workflow" in
  [docs/release.md](docs/release.md), which create a temporary tag to trigger the
  workflow and then delete it again. (This is also why keeping your fork named
  `bmx` is handy — see the fork step above.)

## Documentation

If your change touches something covered in the [`docs/`](docs/) directory,
please keep that documentation up to date, or add a new page there if your work
warrants it. Updating the [CHANGELOG.md](CHANGELOG.md) is optional - 
the maintainers will happily do this for you.

## Coding Style

Please match the style of the surrounding code. Keep changes focused — avoid
unrelated reformatting in the same pull request, as it makes review harder.

## Using AI Tools

Use whatever tools you like, including AI agents — but **own the result**. You
should understand every line, be able to explain it, and be ready to maintain it
after it merges.

Discuss in your own words: review is a human-to-human conversation, not pasted
agent output. Agentic workflows are coming (as of 2026) and we're not against
them, but bmx stays hand-maintained for now — until agents can produce a cleaner
codebase than we can.
