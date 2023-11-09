pipeline {
    agent {
        label common.baseNode()
    }

    stages {
        // stage('Build') {
        //     steps {
        //         sh 'npm install'
        //     }
        // }

        stage('Static Code Analysis') {
            environment {
                SCANNERS               = 'virus,checkmarx,bandit'
                PROJECT_NAME           = 'optimizing-equirectangular-conversion'
                VIRUS_SCAN_DIR         = '.'
                CHECKMARX_PROJECT_NAME = 'optimizing-equirectangular-conversion'
                BANDIT_SOURCE_PATH     = '.'
                BANDIT_SEVERITY_LEVEL  = 'low' 
            }
            steps {
                rbheStaticCodeScan()
            }
        }
    }

    post {
        cleanup {
            cleanWs()
        }
        success {
            script {
                def status_icon = "✅"
                emailext (
                    to: 'douglas.p.bogia@intel.com',
                    subject: "${status_icon} [${currentBuild.currentResult}]- ${env.JOB_NAME} - Build # ${env.BUILD_NUMBER}",
                    body: """<p>Please check console output at:\n${env.RUN_DISPLAY_URL}</p>
                        <p>Details      :</p>
                        <p><b>Job Name</b>- '${env.JOB_NAME}', <b>Build Number</b>- '[${env.BUILD_NUMBER}]'</p>
                        <p>For more details: &QUOT;<a href='${env.BUILD_URL}'>${env.JOB_NAME} [${env.BUILD_NUMBER}]</a>&QUOT;</p>""",
                    attachLog: true,
                    mimeType: 'text/html',
                )
            }
        }
        failure {
            script {
                def status_icon = "❌"
                emailext (
                    to: 'douglas.p.bogia@intel.com',
                    subject: "${status_icon} [${currentBuild.currentResult}]- ${env.JOB_NAME} - Build # ${env.BUILD_NUMBER}",
                    body: """<p>Please check console output at:\n${env.RUN_DISPLAY_URL}</p>
                        <p>Details      :</p>
                        <p><b>Job Name</b>- '${env.JOB_NAME}', <b>Build Number</b>- '[${env.BUILD_NUMBER}]'</p>
                        <p>For more details: &QUOT;<a href='${env.BUILD_URL}'>${env.JOB_NAME} [${env.BUILD_NUMBER}]</a>&QUOT;</p>""",
                    attachLog: true,
                    mimeType: 'text/html',
                )
            }
        }
    }
}