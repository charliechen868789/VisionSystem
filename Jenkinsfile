pipeline {
    agent any

    options {
        timestamps()
        buildDiscarder(logRotator(numToKeepStr: '20'))
    }

    environment {
        BUILD_DIR = 'build'
    }

    stages {
        stage('Install dependencies') {
            steps {
                // Assumes an agent that can apt-get as root (e.g. a Debian/Ubuntu
                // container). Prefix with sudo, or bake these into the agent image,
                // if the agent runs as a non-root user.
                sh '''
                    apt-get update
                    apt-get install -y --no-install-recommends \
                        build-essential cmake pkg-config \
                        libprotobuf-dev protobuf-compiler \
                        libzmq3-dev libevent-dev libcurl4-openssl-dev \
                        libopencv-dev nlohmann-json3-dev
                '''
            }
        }

        stage('Configure') {
            steps {
                sh 'cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON'
            }
        }

        stage('Build') {
            steps {
                sh 'cmake --build "$BUILD_DIR" -j "$(nproc)"'
            }
        }

        stage('Test') {
            steps {
                sh 'cd "$BUILD_DIR" && ctest --output-on-failure --output-junit unit_tests_results.xml'
            }
            post {
                always {
                    junit "${BUILD_DIR}/unit_tests_results.xml"
                }
            }
        }
    }

    post {
        cleanup {
            deleteDir()
        }
    }
}
