pipeline {
  agent any
  
  stages {
    stage('Checkout') {
      steps {
        checkout([$class: 'GitSCM', branches: [[name: '*/main']], doGenerateSubmoduleConfigurations: false, extensions: [], submoduleCfg: [], userRemoteConfigs: [[url: 'https://github.com/tkbstudios/ti84pluscenet-calc']]])
      }
    }
    
    stage('Build') {
      steps {
        sh 'make'
      }
    }
    
    stage('Release') {
      steps {
        script {
          def buildDate = new Date().format('yyyyMMdd')
          def gitCommits = sh(script: 'git log --pretty=format:\'%H\'', returnStdout: true).trim()
          def releaseFileName = "TINET-${buildDate}.8xp"
          
          sh "cp bin/TINET.8xp ${releaseFileName}"
          
          withCredentials([usernamePassword(credentialsId: 'github-creds', passwordVariable: 'password', usernameVariable: 'username')]) {
            sh """
              git config --global user.email "jenkins@jenkins.com"
              git config --global user.name "Jenkins"
              git add ${releaseFileName}
              git commit -m "Automated release for ${buildDate}"
              git push https://${password}@github.com/tkbstudios/ti84pluscenet-calc.git
            """
          }
        }
      }
    }
  }
}