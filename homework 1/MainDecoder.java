import java.io.File;
import java.io.FileNotFoundException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashSet;
import java.util.Scanner;


class MainDecoder
{
	public static void main(String[] args) throws FileNotFoundException, InterruptedException, NoSuchAlgorithmException 
    {		
    	int threads = 0;
		
		// se non specificato in stdin, il numero di thread è pari al numero dei processori della macchina. 
		if (args.length == 1) threads = Runtime.getRuntime().availableProcessors();
		
		else if (args.length == 2) 
		{
			try { threads = Integer.parseInt(args[1]); }
			catch (Exception e) { System.err.println("Wrong usage, " + args[1] + " should be an Integer."); System.exit(1); }
		}
		else
		{
			System.err.println("Wrong number of arguments! \nUsage: java PasswordCracking <digests path> <number of threads>[optional]");
	        System.exit(1);
		}
		
		// legge i digests nel file specificato in stdin
		HashSet<String> digests = new MyFileReader(args[0]).getLines();
		
		System.out.println("\nDecoding passwords from " + args[0] + " file using " + threads + " threads...\n");
		PasswordDecoder.threadCall(digests, threads);
		System.out.println("\nComputation finished. All passwords found!");
	}
}

/**
 * Gestisce la creazione dei thread.
 * @author antoniopioricciardi e matteoprata
 *
 */
class PasswordDecoder
{
	/**
	 * Indica quanti digests andranno ancora analizzati
	 */
	public static int toFind;
	private final static int ALPHABET_DIM = 36;

	public static void threadCall(HashSet<String> digests, int threads) throws InterruptedException
	{
        toFind = digests.size();
		int limit = 1;

		while (toFind != 0)
		{
			threadGenerator(digests, threads, limit);
			limit++;
		}
	}

	/**
	 * Crea un numero di thread pari al valore assunto da <b>threads</b>. 
	 * I thread singolarmente si occuparanno di generare permutazioni di caratteri, ogni thread creerà permutazioni 
	 * ponendo in testa  di volta in volta i caratteri in una sottolista (sotto-alfabeto) delimitata dagli indici (lo, hi).
	 * 
	 * @param digests Set di password da decodificare
	 * @param threads Numero di thread da creare
	 * @throws InterruptedException
	 */
	private static void threadGenerator(HashSet<String> digests, int threads, int limit) throws InterruptedException
	{
        ThreadDecoder[] pt = new ThreadDecoder[threads];       

		for (int i = 0; i < threads; i++)
		{
			// vengono calcolati gli indici per dividere l'array di caratteri in un numero pari al valore di threads
			int lo = (int)((((long)i)*ALPHABET_DIM)/threads);
			int hi = (int)((((long)(i+1))*ALPHABET_DIM)/threads);

			// i threads iniziano la computazione
			pt[i] = new ThreadDecoder(digests, lo, hi, limit);
			pt[i].start();		
		}
        for (int i=0; i < threads; i++) pt[i].join();
	}
}

/**
 * Rappresenta un thread, ogni thread genera permutazioni di caratteri.
 * @author antoniopioricciardi e matteoprata
 *
 */
class ThreadDecoder extends java.lang.Thread
{
	private final char[] alphabet = "abcdefghijklmnopqrstuvwxyz0123456789".toCharArray();
	
	private int lo;
	private int hi;
	private int limit;
	private HashSet<String> digests;
	
	/**
	 * Un thread è definito a partre da delimitatori (lo, hi) che identificano la porzione di alfabeto su cui generare permutazioni; e da un set di digests.
	 * @param digests Un set di digests da cercare
	 * @param lo Delimitato inferiore 
	 * @param hi Delimitazione superiore
	 */
	public ThreadDecoder(HashSet<String> digests, int lo, int hi, int limit) { this.lo = lo; this.hi = hi; this.digests = digests; this.limit = limit;	}
	
	/**
	 * Fintantoché non termina la ricerca di tutti i digests nel file in input, Genera permutazione di parole via via sempre più lunge.
	 */
	public void run()
	{		
		for (int i = lo; i < hi; i++)
		{
			try 
			{
				decode(alphabet[i] + "", limit);
			} 
			catch (Exception e) 
			{
				e.printStackTrace();
			}
		}
	}

	/**
	 * Genera le permutazioni di parole che iniziano per <b>prefix</b>, e controlla che le parole generate siano proprio quelle presenti nel file in input.
	 * @param prefix Il prefisso della permutazione.
	 * @param limit Dimensione massima della permutazione. 
	 * @throws Exception
	 */
	private void decode(String prefix, int limit) throws Exception 
    {
		// il calcolo delle permutazioni si arresta quando tutti i digests nel file di input sono stati decodificati
		if (PasswordDecoder.toFind != 0)
		{
	        if (prefix.length() == limit)
	        {
	        	//System.out.println(prefix);
	            String encoded = MD5Encoder.encode(prefix.toString());   // codifica la permutazione
	            
	            if (digests.contains(encoded))   						 // se la codifica della permutazione è contenuta nel file, allora è la permutazione corretta.
	            {	
        			System.out.println("La password relativa al digest \"" + encoded + "\" e': " + prefix);
        		    PasswordDecoder.toFind--;
	            }
	        }
	        else for (int i = 0; i < alphabet.length; i++) decode(prefix + alphabet[i], limit);   // recursive call
	        
		}
    }
}

/**
 * Legge un file di testo.
 * @author antoniopioricciardi e matteoprata
 */
class MyFileReader 
{
	private String testo = "";
	private HashSet<String> list = new HashSet<String>();
	
	@SuppressWarnings("resource")
	public MyFileReader(String file) throws FileNotFoundException
	{
		Scanner sc = new Scanner(new File(file));
		
		while(sc.hasNext()) 
		{
			String linea = sc.next();
			list.add(linea);
			testo += linea + "\n";
		}
	}
	
	public String getContent() { return testo; }
	public HashSet<String> getLines() { return list; };
}

/**
 * Genera codifiche in MD5.
 * @author antoniopioricciardi e matteoprata
 */
class MD5Encoder {
	
	public static String encode(String password) throws NoSuchAlgorithmException
	{
		MessageDigest md = MessageDigest.getInstance("MD5");
		md.update(password.getBytes());
		byte byteData[] = md.digest();
		
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < byteData.length; i++)
        {
			sb.append(Integer.toString((byteData[i] & 0xff) + 0x100, 16).substring(1));
		}
		return sb.toString();
	}
}

