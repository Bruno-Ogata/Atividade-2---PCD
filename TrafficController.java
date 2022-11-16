/*
Nome: Bruno Hideki Amadeu Ogata
RA: 140884
Programação Concorrente e Distribuida
Atividade 2 - Exercício 3
*/

public class TrafficController {

    private int livre = 0;

    private synchronized void entrar(){
      while(livre == 1){
        try{
          wait();
        }catch(InterruptedException e){
          Thread.currentThread().interrupt();
        }

      }
      
      livre = 1;
      notifyAll();
    }


    private synchronized void sair(){

      livre = 0;
      notifyAll();
    }

    public void enterLeft() {
      entrar();
    }
    public void enterRight() {
      entrar();
    }
    public void leaveLeft() {
      sair();
    }
    public void leaveRight() {
      sair();
    }

}
