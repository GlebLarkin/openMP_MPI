#include <boost/mpi.hpp>
#include <iostream>
#include <vector>

namespace mpi = boost::mpi;

int main(int argc, char *argv[])
{
    mpi::environment env(argc, argv);  // MPI_Init / env -> MPI_Finalize
    mpi::communicator world;           // MPI_COMM_WORLD

    if (world.size() < 2) {
        if (world.rank() == 0)
            std::cerr << "need at least 2 processes\n";
        return 1;
    }

    const int tag = 0;
    const int n = 16;
    std::vector<char> buf(static_cast<std::size_t>(n), 'A');

    // Массив char* + длина -> один MPI_Send / MPI_Recv, без сериализации.
    // world.send(1, tag, buf) шлёт ещё и размер — для замеров ДЗ не годится.
    if (world.rank() == 0) {
        world.send(1, tag, buf.data(), n);
        world.recv(1, tag, buf.data(), n);
        std::cout << "ok, ping-pong " << n << " bytes\n";
    } else if (world.rank() == 1) {
        world.recv(0, tag, buf.data(), n);
        world.send(0, tag, buf.data(), n);
    }

    // Ssend / Bsend в Boost.MPI нет. communicator неявно = MPI_Comm:
    // MPI_Ssend(buf.data(), n, MPI_CHAR, dest, tag, world);
    // MPI_Buffer_attach(...);
    // MPI_Bsend(buf.data(), n, MPI_CHAR, dest, tag, world);

    return 0;
}
