import java.util.Queue;
import java.util.LinkedList;

public class Gamemode {
	private static int gamemode=0;
	private static Volitile vol;

	/*Set gamemode. returns -1 when an error occurs.*/
	public static int set(int gm) {
		if (gamemode == gm || (gm != 0 && gm != 1))
			return -1;
	
		switch (gm) {
		case 0: //reset
			switch (gamemode) {
			case 1:
				vol=null; break;
			}
			break;

		case 1: //vol
			vol = new Volitile();
			break;
		}
		
		gamemode=gm;
		return 0;
	}

	/*Put a piece. Returns coord to delete, -1 when there's nothing.*/
	public static int put(int coord) {
		switch (gamemode) {
		case 1:
			return vol.put(coord);
		}

		return -1;
	}

	public static void reset() {
		switch (gamemode) {
			case 1:
				vol.reset();
			}
	}
}

class Volitile {
	int limit;
	Queue<Integer> q;
	int piececount;
	
	Volitile() {
		limit=16;
		q = new LinkedList<>();
	}

	/*Put a piece. returns coord to del, -1 when there's nothing delete.*/
	int put(int coord) {
		int todelete=-1;

		if (++piececount > limit)
			todelete=q.poll();
		
		q.offer(coord);

		return todelete;
	}

	void reset() {
		q=new LinkedList<>();
	}
}