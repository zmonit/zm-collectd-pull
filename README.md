# zm-collectd-pull
Pull data from collectd daemon to zmonit

# Building zm-collectd-pull

To use or contribute to zm-collectd-pull, build and install this stack:

    git clone git://github.com/jedisct1/libsodium.git
    git clone git://github.com/zeromq/libzmq.git
    git clone git://github.com/zeromq/czmq.git
    git clone git://github.com/zeromq/malamute.git
    git clone git://github.com/zmonit/zm-proto.git
    git clone git://github.com/collectd/collectd.git
    for project in libsodium libzmq czmq malamute zm-proto collectd.git; do
        cd $project
        ./autogen.sh
        ./configure && make check
        sudo make install
        sudo ldconfig
        cd ..
    done

To run zm-collectd-pull, issue this command:

    zm-collectd-pull

To end the broker, send a TERM or INT signal (Ctrl-C).

# unimplemented
1. tests
2. actor API
3. configurator for collectd
