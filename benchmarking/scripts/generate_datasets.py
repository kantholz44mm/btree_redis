#!/usr/bin/env python3
import generate_integer_datasets
import generate_random_url_dataset
import generate_wikipedia_dataset


def main():
    print("=== Integer datasets ===")
    generate_integer_datasets.main()

    print("\n=== Wikipedia titles ===")
    generate_wikipedia_dataset.main()

    print("\n=== Common Crawl URLs ===")
    generate_random_url_dataset.main()


if __name__ == "__main__":
    main()
