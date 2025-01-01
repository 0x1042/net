import logging
import server
import typer

if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s - %(thread)d - %(levelname)s - %(message)s",
        level=logging.DEBUG,
    )

    typer.run(server.run)
