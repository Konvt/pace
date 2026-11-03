# Contributing
Contributions are welcome! Whether it's a bug fix, a new feature, documentation improvement, or just a small correction, every contribution is appreciated.

This document describes a few conventions that help keep contributions easy to review and maintain.

## Before You Start
For small and straightforward changes, feel free to submit a pull request directly. This includes:

- Typo and grammar fixes.
- Documentation improvements and translations.
- Comment corrections.
- Minor code cleanups.
- Small, self-contained fixes.

For more substantial changes, it's preferable to open an issue first and discuss the proposed approach before starting implementation.

This is particularly useful for:

- Non-trivial bug fixes.
- New features or significant behavior changes.
- Backports or alternative implementations of existing functionality.
- Public API changes.
- Changes to existing abstractions or extension mechanisms.
- Changes that may affect compatibility or the overall architecture.

A short discussion beforehand can help clarify the intended behavior and avoid spending time on an implementation that may not fit the project.

## Development Guidelines
Please try to follow the existing conventions and design principles of the project.

A few general preferences:

- Prefer existing abstractions and mechanisms where appropriate.
- Avoid unnecessary complexity and duplicated functionality.
- Keep changes focused and reasonably self-contained.
- Consider compatibility and long-term maintainability.
- Avoid introducing unrelated changes into a pull request.

If your changes introduce a new abstraction or take a non-obvious approach, a brief explanation of the reasoning would be helpful.

## Building and Verification
The project currently does not have a dedicated test suite.

At a minimum, please make sure that the affected demo files still compile successfully after your changes.

If your changes cannot be verified through the existing demos, please mention what you have checked and any remaining limitations in your pull request.

## Branches and Pull Requests
For changes that are relatively small or do not require a separate release, please target the `nightly` branch whenever possible.

Changes intended for a stable release or requiring a different integration path can target another appropriate branch. If you are unsure, feel free to ask.

Please keep pull requests focused and avoid mixing unrelated modifications.

### Commit History
There is no strict requirement for a linear commit history.

However, please try to keep the commit history reasonably clean and understandable.

A few preferences:

- Avoid unnecessary merge commits in pull requests.
- Prefer clear and meaningful commit messages.
- Avoid committing unrelated changes.
- Squashing commits is fine when it makes the history easier to follow.

You do not need to rewrite your history just to satisfy a particular commit structure. The goal is simply to keep the history readable without introducing unnecessary complexity.

## Review
I'll review contributions with the project's overall design and maintainability in mind.

Some submitted implementations may need adjustments or refactoring before they are integrated. This is perfectly normal, and I may make further changes after merging a contribution to better fit the existing architecture.

If I have suggestions or concerns, I'll try to explain the reasoning so that the intended direction is clear.

## Questions
If you are unsure about an implementation approach or how something fits into the existing architecture, feel free to open an issue for discussion.

Thanks for taking the time to contribute!
